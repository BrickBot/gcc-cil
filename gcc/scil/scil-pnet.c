#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "toplev.h"

#include "scil-config.h"
#include "scil-cral.h"
#include "scil-callbacks.h"
#include "scil-instructions.h"
#include "scil-builtins.h"

#include "il_image.h"
#include "il_program.h"
#include "program.h"





/* Prototypes for instruction functions */
#define SCIL_INSTRUCTION(number, name, suffix) static void instr_##suffix (void);
#include "scil-instructions.def"
#include "scil-extended-instructions.def"
#undef SCIL_INSTRUCTION

/* Definition of the instructions arrays */
typedef void (*instruction_function_type) (void);

#define SCIL_INSTRUCTION(number, name, suffix) instr_##suffix,
static instruction_function_type instructions[] = 
{
#include "scil-instructions.def"
};

static instruction_function_type extended_instructions[] = 
{
#include "scil-extended-instructions.def"
};
#undef SCIL_INSTRUCTION

/* Definition of the instruction names arrays*/
#define SCIL_INSTRUCTION(number, name, suffix) name,
static const char *instruction_names [] =
{
#include "scil-instructions.def"
};

static const char *extended_instruction_names [] =
{
#include "scil-extended-instructions.def"
};
#undef SCIL_INSTRUCTION



/* Helper macros */
#define get_tree(item) \
            (tree) ILProgramItemGetUserData (ILToProgramItem ((item)))
#define set_tree(item, data) \
            ILProgramItemSetUserData (ILToProgramItem ((item)), (data))

#define resolve_member(item) \
  (ILMemberResolve (ILProgramItemToMember (ILToProgramItem (item))))
#define resolve_method(item) \
  (ILProgramItemToMethod (ILToProgramItem (resolve_member (item))))
#define resolve_field(item) \
  (ILProgramItemToField (ILToProgramItem (resolve_member (item))))



/* Function prototypes */
static void check_for_memory_alias (ILField *);
static tree get_class_tree (ILClass *);
static ILAttribute *get_next_attribute (ILProgramItem *, ILAttribute *, const char *, const char *);
static tree get_type_tree (ILType *);
static tree get_var_type_tree (ILType *);
static int  is_static_delegate (ILClass *);
static void load_file (const char *, ILImage **);
static void parse_vtable (ILClass *);
static void parse_instance_fields (ILClass *);
static void parse_method (ILMethod *);
static tree register_class (ILClass *);
static tree register_method (ILMethod *);
static tree register_method_type (ILMethod *, tree);
static tree register_method_type_direct (ILType *, tree);
static tree register_static_field (ILClass *, ILField *);

static void defer_method (ILMethod *);
static bool parse_next_deferred_method (void);



/* File global variables */
static ILContext *context;
/* IMAGE is the image that contains the entry point, CURRENT_IMAGE is
   the image that the method is in we are currently parsing. */
static ILImage *image, *current_image;
static ILUInt8 *code;
static ILUInt32 code_length;
static ILUInt32 pc;

/* Arrays for parameters and locals.
   We need not care about the ggc because all their elements are referenced
   indirectly by the method tree: each parameter is in the DECL_ARGUMENTS chain,
   locals are in the BLOCK_VARS chain of the methods main BLOCK. Thus the ggc 
   will never remove them and we don't need GTY markers here.
*/
static tree *parameters = NULL;
static tree *locals = NULL;


/* When compiling methods we will encounter calls to methods. We put them in
   this queue and compile them after the method currently compiled if 
   necessary.
*/
struct deferred_method_t
{
  ILMethod *method;
  struct deferred_method_t *next;
};
static struct deferred_method_t *first_deferred_method = NULL;
static struct deferred_method_t *last_deferred_method = NULL;


/* order in this array is significant */
tree *native_type_trees[] =
{
  0, /* not a valid type */
  &void_type_node,
  &scil_bool_type,
  &scil_char_type,
  &scil_int8_type,
  &scil_uint8_type,
  &scil_int16_type,
  &scil_uint16_type,
  &scil_int32_type,
  &scil_uint32_type,
  &scil_int64_type,
  &scil_uint64_type,
  &scil_float32_type,
  &scil_float64_type,
  &scil_string_type,
  0, /* TODO: ELEMTYPE_PTR*/
  0, /* TODO: ELEMTYPE_BYREF */
  0, /* TODO: ELEMTYPE_VALUETYPE */
  0, /* TODO: ELEMTYPE_CLASS */
  0, /* TODO: ELEMTYPE_VAR */
  0, /* TODO: ELEMTYPE_ARRAY */
  0, /* TODO: ELEMTYPE_WITH */
  0, /* TODO: ELEMTYPE_TYPEDBYREF */
  0, /* TODO: ELEMTYPE_VALUEARRAY */
  &scil_native_int_type,
  &scil_native_uint_type,
  0, /* TODO: ELEMTYPE_R */
  0, /* TODO: ELEMTYPE_FNPTR */
  &scil_object_type,
  0, /* TODO: ELEMTYPE_SZARRAY */
  0, /* TODO: ELEMTYPE_MVAR */
  0, /* TODO: ELEMTYPE_CMOD_REQD */
  0, /* TODO: ELEMTYPE_CMOD_OPT */
  0, /* TODO: ELEMTYPE_INTERNAL */
  0, /* TODO: ELEMTYPE_VAR_OLD */
  0, /* TODO: ELEMTYPE_NAME */
  0, /* TODO: ELEMTYPE_MODIFIER */
  0, /* TODO: ELEMTYPE_SENTINEL */
  0, /* TODO: ELEMTYPE_PINNED */
};

/* see scil-cral.h */
bool
cral_init ()
{
  context = ILContextCreate ();

  return true;
}


/* see scil-cral.h */
void
cral_finalize ()
{
  ILContextDestroy(context);
}


/* see scil-cral.h */
void 
cral_set_search_directories (int num, char **dirs)
{  
  ILContextSetLibraryDirs (context, dirs, num);
}


/* see scil-cral.h */
void
cral_load_system_library (const char *name)
{
  ILImage *mscorlib;
  load_file (name, &mscorlib);

  ILContextSetSystem (context, mscorlib);
}


/* see scil-cral.h */
void 
cral_parse_file (const char *filename)
{

  load_file (filename, &image);

  /* parse entry point and all directly or indirectly methods and classes */
  ILMethod *entry = ILMethod_FromToken (image, ILImageGetEntryPoint (image));
  if (entry)
    {
      register_method (entry);
      parse_method (entry);
      while (parse_next_deferred_method ());
    
      scil_set_entry_point (get_tree (entry));
    }
}

void 
load_file (const char *filename, ILImage **image)
{
  int load_error = ILImageLoadFromFile (filename, context, image, 0, 0);
  if (load_error)
    {
      error ("%s: %s", filename, ILImageLoadError (load_error));
    }  
}


/* Registers IL_CLASS with the front end and stores the generated tree in the
   user data field of the ILProgramItem. Returns that tree.
*/
tree
register_class (ILClass *il_class)
{
  const char *class_name;
  ILClass *nesting_parent;
  tree type;

  /* register only if necessary */
  type = get_tree (il_class);
  if (type != NULL_TREE)
    return type;


  nesting_parent = ILClass_NestedParent (il_class);  

  tree container;
  if (nesting_parent)
    {
      container = get_class_tree (nesting_parent);
    }
  else
    {
      tree a = scil_register_assembly ( 
        ILImageGetAssemblyName (ILClassToImage (il_class)));
    
      container = scil_register_namespace (a, ILClassGetNamespace (il_class));
    }

  class_name = ILClassGetName (il_class);
  type = scil_register_class (container, class_name);
  set_tree (il_class, type);
   
  return type;
}


void
parse_instance_fields (ILClass *il_class)
{
  ILMember *member;
  ILClass *base;
  tree type;

  il_class = ILClassResolve (il_class);

  type = register_class (il_class);

  if (SCIL_TYPE_PARSED (type))
    return;
  
  /* make sure base class's instance fields have been parsed */
  if (!ILClassIsValueType (il_class)
      && !is_static_delegate (il_class))
    {
      base = ILClassGetParent (il_class);
      if (base) 
        {
          parse_instance_fields (base);
          scil_set_base_class (type, get_tree (base));
        }
    }

  /* add all fields */
  member = 0;
  while ((member = ILClassNextMemberByKind (il_class, member, 
                                            IL_META_MEMBERKIND_FIELD)))
    {
      ILField *field;
    
      field = resolve_field (member);
      if (field && !ILField_IsStatic (field))
        {
          tree ftype;
        
          ftype = get_var_type_tree (ILField_Type (field));
          set_tree (field, scil_add_field (type, ILField_Name (field), ftype));
        
          check_for_memory_alias (field);
        }
    }

  scil_complete_type (type);
}

tree
register_static_field (ILClass *il_class, ILField *il_field)
{
  tree type, ftype, field;
  ILMethod *class_ctor;

  /* Register and parse the static constructor */
  class_ctor = resolve_method (ILClassNextMemberMatch (il_class, 0, IL_META_MEMBERKIND_METHOD, ".cctor", 0));
  if (class_ctor)
    {
      scil_mark_class_constructor (register_method (class_ctor));
      defer_method (class_ctor);
    }

  type = register_class (il_class);
  ftype = get_var_type_tree (ILField_Type (il_field));
  field = scil_register_static_field (type, ILField_Name (il_field), ftype);
  set_tree (il_field, field);

  check_for_memory_alias (il_field);

  return field;
}

#define has_attribute(item, name, namespace) \
  get_next_attribute (ILToProgramItem (item), 0, name, namespace) != 0


/* Returns the next attribute after LAST_ATTR for ITEM that is of the type
   given by NAME and NAME_SPACE */
ILAttribute *
get_next_attribute (ILProgramItem *item, ILAttribute *last_attr, const char *name, const char *name_space)
{
  ILClass *attr_class;

  attr_class = ILClassResolve (ILClassLookupGlobal (context, name, name_space));

  while ((last_attr = ILProgramItemNextAttribute (item, last_attr)))
    {
      if (ILAttributeTypeIsItem (last_attr))
        {
          ILClass *klass;
          klass = ILClassResolve (ILMethod_Owner (
                    ILProgramItemToMethod (ILAttributeTypeAsItem (last_attr))));
    
          if (klass == attr_class)
            {
              return last_attr;
            }
        }
    }
  
  return 0;
}

void
check_for_memory_alias (ILField *il_field)
{
  ILAttribute *attr;
  ILProgramItem *item;

  item = ILToProgramItem (il_field);
  attr = get_next_attribute (item, 0, "MemoryAliasAttribute", "Hardware");
  if (attr)
    {
      unsigned long len;
      ILUInt8 *blob = (ILUInt8 *) ILAttributeGetValue (attr, &len);
      
      /* TODO: support all MemoryAliasAttribute ctors */
      scil_add_memory_alias (get_tree (il_field), IL_READ_INT32 (&blob[2]));
    }
}

int
is_static_delegate (ILClass *il_class)
{
  return has_attribute (il_class, "StaticDelegateAttribute", "Hardware");
}


void
parse_vtable (ILClass *il_class)
{
  ILMember *member;
  ILMethod *method;
  ILClass *base;
  tree type;
  unsigned int vcount = 0;
  
  type = register_class (il_class);


  if (ILClassIsValueType (il_class)
      || is_static_delegate (il_class))
    return;
    
  if (SCIL_VTABLE_PARSED (type))
    return;

  /* make sure base class's virtual-method slots have been parsed */ 
  base = ILClassGetParent (il_class);
  if (base) 
    {
      parse_vtable (base);
      scil_set_base_class (type, get_tree (base));
    }

  /* count newly defined virtual methods before adding them. it seems a bit
     clumsy to have this loop twice but consider the effort for additional 
     dynamic memory allocation. 
  */
  member = 0;
  while ((member = ILClassNextMemberByKind (il_class, member, 
                                            IL_META_MEMBERKIND_METHOD)))
    {    
      method = resolve_method (member);
      if (method && ILMethod_IsVirtual (method) && !ILMemberGetBase (member))
        { 
          ++vcount;
        }
    }

  scil_make_vtable (type, vcount);

  /* set vtable slots and add new ones where needed*/
  member = 0;
  while ((member = ILClassNextMemberByKind (il_class, member, 
                                            IL_META_MEMBERKIND_METHOD)))
    {
      method = resolve_method (member);
      if (method && ILMethod_IsVirtual (method))
        {
          ILMethod *parent;
          
          parent = resolve_method (ILMemberGetBase (member));
          method->index = parent ? 
                          parent->index : 
                          scil_new_vtable_slot (type);
          scil_set_vtable_slot (type, method->index, register_method (method));
          defer_method (method);
        }
    }
  
  scil_complete_vtable (type);
}

/* INSTANCE_TYPE should be NULL_TREE for static methods. */
tree
register_method_type_direct (ILType *signature, tree instance_type)
{
  tree return_type, type;
  bool varargs_p;
  unsigned long num_params, p;

  /* create method type with return type*/
  return_type = get_var_type_tree (ILTypeGetReturn (signature));
  type = scil_make_method_type (return_type);

  /* add other parameter types */
  num_params = ILTypeNumParams (signature);
  for (p = 1; p <= num_params; ++p)
    {
      ILType *ptype;
      tree ptree;
      bool byref;
      
      ptype = ILTypeGetParam (signature, p);
      ptree = get_var_type_tree (ptype);
    
      /* TODO: extend to everything which is a reference */
      byref = false;
    
      scil_add_method_parameter_type (type, ptree, byref);
    }

  /* resolve if method type has varargs and complete it */
  varargs_p = (IL_META_CALLCONV_VARARG == ILType_CallConv(signature));
  return scil_complete_method_type (type, instance_type, varargs_p);
}

tree
register_method_type (ILMethod *il_method, tree owner)
{
  owner = ILMethod_IsStatic (il_method) ? NULL_TREE : owner;
  return register_method_type_direct (ILMethod_Signature (il_method), owner);
}

tree
register_method (ILMethod *il_method)
{
  tree type, owner, method;


  /* register only if necessary */
  method = get_tree (il_method);
  if (method != NULL_TREE)
    return method;

  /* register class if necessary */
  owner = get_class_tree (ILMethod_Owner (il_method));

  type = register_method_type (il_method, owner);

  /* register method with its container, 
     always presuming non-runtime methods */
  method = scil_register_method (owner, ILMethod_Name (il_method), type);
  set_tree (il_method, method);
    
  return method;
}


void
parse_method (ILMethod *il_method)
{
  tree method;
  ILType *signature;
  ILMethodCode body;
  unsigned long num_parameters, p;
  ILParameter *parameter;
  bool static_p;

  current_image = ILProgramItemGetImage (ILToProgramItem (il_method));
  method = get_tree (il_method);
  signature = ILMethod_Signature (il_method);

  /* If method is not defined in CIL emit error if not a runtime method. */
  if (!ILMethodGetCode (il_method, &body))
    {
      if (scil_start_method_def (method, 0))
        error ("Method %s (%s) contains no or malformed code and has not been declared by the runtime.",
               IDENTIFIER_POINTER (DECL_NAME (method)), 
               IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (method)));
      return;
    }
  
  /* If method has been defined before, skip it silently. */
  if (!scil_start_method_def (method, body.maxStack))
    return;

# ifdef SCIL_TRACE_VIRTUAL_EXECUTION
    fnotice (stderr, "compiling %s\n", IDENTIFIER_POINTER( DECL_ASSEMBLER_NAME (method)));
# endif
  
  /* Check for attributes of extentions for hardware */
  if (has_attribute (il_method, "SaveRegistersAttribute", "Hardware"))
    {
      scil_add_attribute (method, "saveall");
    }
  
  if (has_attribute (il_method, "NoReturnAttribute", "Hardware"))
    {
      scil_add_attribute (method, "noreturn");
    }
  
  if (has_attribute (il_method, "InterruptHandlerAttribute", "Hardware"))
    {
      /* different back-ends use different names for interrupt handlers */
      scil_add_attribute (method, "interrupt");
      scil_add_attribute (method, "interrupt_handler");
    }
  
  static_p = ILMethod_IsStatic(il_method);
  
  /* parse parameters, register them with the front end and put them in an
     array PARAMETERS for later use with the instructions */
  
  num_parameters = ILTypeNumParams (signature);
  if (!static_p) ++num_parameters;
  parameters = (tree*) xmalloc (num_parameters * sizeof (tree));
 
  if (static_p)
    {
      p = 0;
    }
  else
    {
      parameters[0] = scil_add_method_parameter (SCIL_ID_THIS);
      p = 1;
    }
  
  parameter = 0;
  while ((parameter = ILMethodNextParam(il_method, parameter)))
    {
      const char *pname = ILParameter_Name (parameter);
      parameters[p++] = scil_add_method_parameter (pname);
    }

  
  /* parse local variables, register them with the front end and put them in an
     array LOCALS for later use with the instructions */
  
  if (body.localVarSig != 0) 
    {
      ILType *local_sig = ILStandAloneSigGetType (body.localVarSig);
      unsigned long num_locals = ILTypeNumLocals (local_sig);
      locals = (tree*) xmalloc (num_locals * sizeof (tree));
  
      unsigned long l = -1; 
      while (++l < num_locals)
        {
          tree ltype;
          
          ltype = get_var_type_tree (ILTypeGetLocal (local_sig, l));
          locals[l] = scil_add_method_variable (ltype);
        }
      
      /* TODO: initialize locals to 0 (integers) or 0.0 (floats) if localsinit flag is set
               or call initializer for value types
      */
    }
    
  /* Parse instructions */
  if (body.code)
    {
      code = body.code;
      code_length = body.codeLen;
      pc = 0;
      while (pc < code_length)
        {
#         ifdef SCIL_TRACE_VIRTUAL_EXECUTION
            fnotice (stderr, "execute 0x%x: %s\n", pc, instruction_names [code [pc]]);
#         endif
          scil_set_label (pc);
          instructions [code[pc++]]();
        }
    }
  scil_complete_method_def ();
  
  if (parameters != NULL) 
    {
      free (parameters);
      parameters = NULL;
    }
  if (locals != NULL) 
    {
      free (locals);
      locals = NULL;
    }
}


void
defer_method (ILMethod *method)
{
  struct deferred_method_t *m;
  
  m = (struct deferred_method_t *) xmalloc(sizeof (struct deferred_method_t));
  m->method = method;
  m->next = NULL;

  if (first_deferred_method == NULL)
    {
      first_deferred_method = m;
      last_deferred_method = m;
    }
  else
    {
      last_deferred_method->next = m;
      last_deferred_method = m;
    }
}

bool
parse_next_deferred_method ()
{
  struct deferred_method_t *method;

  if (first_deferred_method == NULL)
    return false;
  
  method = first_deferred_method;
  first_deferred_method = first_deferred_method->next;
  
  parse_method (method->method);
  
  free (method);
  return true;
}




/* GENERAL HELPERS */
tree
get_class_tree (ILClass *il_class)
{
  tree t;

  il_class = ILClassResolve (il_class);
  t = get_tree (il_class);
  if (!t) 
    {
      register_class (il_class);
      t = get_tree (il_class);
    }
  return t;
}


/* Finds the tree for the IL_TYPE. */
tree
get_type_tree (ILType *il_type)
{
  /* TODO: extend for handling all types */
  
  if (ILType_IsPrimitive (il_type))
      return *native_type_trees [ILType_ToElement (il_type)];

  /* Make sure the fields of this type are known as we need the type's size. */  
  if (ILType_IsValueType (il_type))
    {
      ILClass *type;
    
      type = ILType_ToValueType (il_type);
      parse_instance_fields (type);      
      return get_class_tree (type);
    }
  
  if (ILTypeIsDelegate (il_type))
    {
      ILClass *type;
    
      type = ILType_ToClass (il_type);
      if (is_static_delegate (type))
        return ptr_type_node;
      
      return get_class_tree (type); 
    }

  if (ILType_IsClass (il_type))
    return get_class_tree (ILType_ToClass (il_type));
  
  sorry ("Type %x.", il_type);
  return error_mark_node;  
}

/* Returns the "real" type of variables, hence the same as get_type_tree but 
   returns a pointer in case of classes. */
tree
get_var_type_tree (ILType *il_type)
{
  tree t;

  t = get_type_tree (il_type);
  
  if (ILType_IsClass (il_type)
      && !is_static_delegate (ILType_ToClass (il_type)))
      return SCIL_POINTER_TYPE (t);
  
  return t;
}



/* HELPERS FOR INSTRUCTION FUNCTIONS */

static ILInt8
read_int8 (void)
{
  return (ILInt8) code[pc++];
}

static ILInt16 ATTRIBUTE_UNUSED
read_int16 (void)
{
  ILInt16 i = IL_READ_INT16 (&code[pc]);
  pc += 2;
  return i;
}

static ILInt32
read_int32 (void)
{
  ILInt32 i = IL_READ_INT32 (&code[pc]);
  pc += 4;
  return i;
}

static ILInt64
read_int64 (void)
{
  ILInt64 i = IL_READ_INT64 (&code[pc]);
  pc += 8;
  return i;
}

static ILUInt8 ATTRIBUTE_UNUSED
read_uint8 (void)
{
  return code[pc++];
}

static ILUInt16
read_uint16 (void)
{
  ILInt16 i = IL_READ_UINT16 (&code[pc]);
  pc += 2;
  return i;
}

static ILUInt32
read_uint32 (void)
{
  ILInt32 i = IL_READ_UINT32 (&code[pc]);
  pc += 4;
  return i;
}

static ILUInt64 ATTRIBUTE_UNUSED
read_uint64 (void)
{
  ILInt64 i = IL_READ_UINT64 (&code[pc]);
  pc += 8;
  return i;
}




static ILToken
read_token (void)
{
  return (ILToken) read_uint32 ();
}

static tree
read_field (void)
{
  ILField *f;
  tree field;

  f = resolve_field (ILField_FromToken(current_image, read_token()));
  field = get_tree (f);
  
  if (field == NULL_TREE)
    {
      ILClass *il_class;
    
      il_class = ILField_Owner (f);
      if (ILField_IsStatic (f))
          return register_static_field (il_class, f);

      parse_instance_fields (il_class);
      field = get_tree (f);
    }

  return field;
}

static ILMethod *
read_method (void)
{
  ILMethod *m;
  tree method;

  m = resolve_method (ILMethod_FromToken(current_image, read_token()));
  method = get_tree (m);

  if (method == NULL_TREE
     && !is_static_delegate (ILMethod_Owner (m)))
    {
      register_method (m);
      defer_method (m);
    }

  return m;
}


static unsigned int
read_target_short (void)
{
  ILInt8 offset = read_int8 ();
  return (unsigned int) offset + pc;
}

static unsigned int
read_target_long (void)
{
  ILInt32 offset = read_int32 ();
  return (unsigned int) offset + pc;
}


/* INSTRUCTION FUNCTIONS */

/* General notes on instruction functions:
   
   * At the begin of each instruction function, the program counter PC points 
     to the next byte after the instruction's opcode. At the end of each
     instruction function, PC must point to the opcode of the next instruction.
     Hence for one-byte instructions PC must not be manipulated whereas
     for longer instruction you have to. If you use any of the read_... 
     functions, they do this job for you.
     

*/

#define DEFINE_INSTR_NOT_IMPLEMENTED(name) \
  void \
  instr_##name () \
  { \
    sorry (instruction_names [code [pc - 1]]); \
  }


void
instr_nop ()
{
  /* nothing to do */
}


void
instr_break ()
{
  /* TODO: implement debugging support */
  static bool informed = false;
  if (!informed)
  {    
    fnotice (stderr, "Breakpoints have not been implemented yet.\n");
    informed = true;
  }
}

#define DEFINE_INSTR_LD(cod,num,array) \
  void \
  instr_ld##cod##_##num () \
  { \
    scil_instr_push (array[num]);\
  }

#define DEFINE_INSTR_LDARG(num) DEFINE_INSTR_LD(arg,num,parameters)
#define DEFINE_INSTR_LDLOC(num) DEFINE_INSTR_LD(loc,num,locals)

#define DEFINE_INSTR_STLOC(num) \
  void \
  instr_stloc_##num () \
  { \
    scil_instr_pop_into (locals[num]);\
  }


DEFINE_INSTR_LDARG(0);
DEFINE_INSTR_LDARG(1);
DEFINE_INSTR_LDARG(2);
DEFINE_INSTR_LDARG(3);

DEFINE_INSTR_LDLOC(0);
DEFINE_INSTR_LDLOC(1);
DEFINE_INSTR_LDLOC(2);
DEFINE_INSTR_LDLOC(3);

DEFINE_INSTR_STLOC(0);
DEFINE_INSTR_STLOC(1);
DEFINE_INSTR_STLOC(2);
DEFINE_INSTR_STLOC(3);


void
instr_ldarg_s ()
{
  scil_instr_push (parameters[code[pc++]]);
}


void
instr_ldarga_s ()
{
  scil_instr_push_address (parameters[code[pc++]]);
}


void
instr_starg_s ()
{
  scil_instr_pop_into (parameters[code[pc++]]);
}


void
instr_ldloc_s ()
{
  scil_instr_push (locals[code[pc++]]);
}


void
instr_ldloca_s ()
{
  scil_instr_push_address (locals[code[pc++]]);
}


void
instr_stloc_s ()
{
  scil_instr_pop_into (locals[code[pc++]]);
}


void
instr_ldnull ()
{
  scil_instr_push (null_pointer_node);
}

#define DEFINE_INSTR_LDC_I4(num) \
  void \
  instr_ldc_i4_##num () \
  { \
    scil_instr_push_integer (num, scil_int8_type);\
  }

void
instr_ldc_i4_m1 ()
{
  scil_instr_push_integer (-1, scil_int8_type);
}

DEFINE_INSTR_LDC_I4(0);
DEFINE_INSTR_LDC_I4(1);
DEFINE_INSTR_LDC_I4(2);
DEFINE_INSTR_LDC_I4(3);
DEFINE_INSTR_LDC_I4(4);
DEFINE_INSTR_LDC_I4(5);
DEFINE_INSTR_LDC_I4(6);
DEFINE_INSTR_LDC_I4(7);
DEFINE_INSTR_LDC_I4(8);

void
instr_ldc_i4_s ()
{
  scil_instr_push_integer (code[pc++], scil_int8_type);
}


void
instr_ldc_i4 ()
{
  ILInt32 i;
  tree type;

  i = read_int32 ();
  
  type = -128 <= i && i <= 127 ? scil_int8_type :
         -32768 <= i && i <= 32767 ? scil_int16_type :
         scil_int32_type;

  scil_instr_push_integer (i, type);
}


void
instr_ldc_i8 ()
{
  ILInt64 i;
  tree type;

  i = read_int64 ();
  
  type = -0x80 <= i && i <= 0x7f ? scil_int8_type :
         -0x8000 <= i && i <= 0x7fff ? scil_int16_type :
         -0x80000000 <= i && i <= 0x7fffffff ? scil_int32_type :
         scil_int64_type;

  scil_instr_push_integer (i, type);
}


DEFINE_INSTR_NOT_IMPLEMENTED (ldc_r4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldc_r8)


#define DEFINE_INSTR_NOARG(op) \
  void \
  instr_##op () \
  { \
    scil_instr_##op (); \
  } 

DEFINE_INSTR_NOARG(dup)
DEFINE_INSTR_NOARG(pop)

DEFINE_INSTR_NOT_IMPLEMENTED (jmp)

static void
call_delegate (ILClass *del_class, ILType *del_type, ILMethod *method)
{
  if (method == ILTypeGetDelegateMethod (del_type))
    {
      if (is_static_delegate (del_class))
        {
          scil_instr_call_indirect (register_method_type (method, NULL_TREE));
        }
      else
        {
          sorry ("Delegates without StaticDelegateAttribute");
        }
    }
  else if (method == ILTypeGetDelegateBeginInvokeMethod (del_type))
    {
      sorry ("BeginInvoke method for delegates");
    }
  else if (method == ILTypeGetDelegateEndInvokeMethod (del_type))
    {
      sorry ("EndInvoke method for delegates");
    }
  else
    {
      gcc_unreachable ();
    }
}


void
instr_call ()
{
  ILMethod *method;
  ILClass *owner;
  ILType *type;

  method = read_method ();
  owner = ILMethod_Owner (method);
  type = ILClassToType (owner);

  if (ILTypeIsDelegate (type))
    {
      call_delegate (owner, type, method);
    }
  
  scil_instr_call (get_tree (method));
}


DEFINE_INSTR_NOT_IMPLEMENTED (calli)

DEFINE_INSTR_NOARG (ret)


/* Branch instructions */

#define DEFINE_INSTR_BRANCH_S(op) \
  void \
  instr_b##op##_s () \
  { \
    scil_instr_compare_##op ();\
    scil_instr_branch_true (read_target_short ());\
  }

DEFINE_INSTR_BRANCH_S (eq)
DEFINE_INSTR_BRANCH_S (ge)
DEFINE_INSTR_BRANCH_S (gt)
DEFINE_INSTR_BRANCH_S (le)
DEFINE_INSTR_BRANCH_S (lt)
DEFINE_INSTR_BRANCH_S (ne_un)
DEFINE_INSTR_BRANCH_S (ge_un)
DEFINE_INSTR_BRANCH_S (gt_un)
DEFINE_INSTR_BRANCH_S (le_un)
DEFINE_INSTR_BRANCH_S (lt_un)

#define DEFINE_INSTR_BRANCH(op) \
  void \
  instr_b##op () \
  { \
    scil_instr_compare_##op ();\
    scil_instr_branch_true (read_target_long ());\
  }

DEFINE_INSTR_BRANCH (eq)
DEFINE_INSTR_BRANCH (ge)
DEFINE_INSTR_BRANCH (gt)
DEFINE_INSTR_BRANCH (le)
DEFINE_INSTR_BRANCH (lt)
DEFINE_INSTR_BRANCH (ne_un)
DEFINE_INSTR_BRANCH (ge_un)
DEFINE_INSTR_BRANCH (gt_un)
DEFINE_INSTR_BRANCH (le_un)
DEFINE_INSTR_BRANCH (lt_un)

void
instr_br_s ()
{
  scil_instr_branch (read_target_short ());
}


void
instr_brfalse_s ()
{
  scil_instr_branch_false (read_target_short ());
}


void
instr_brtrue_s ()
{
  scil_instr_branch_true (read_target_short ());
}

void
instr_br ()
{
  scil_instr_branch (read_target_long ());
}


void
instr_brfalse ()
{
  scil_instr_branch_false (read_target_long ());
}


void
instr_brtrue ()
{
  scil_instr_branch_true (read_target_long ());
}


DEFINE_INSTR_NOT_IMPLEMENTED (switch)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_i1)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_u1)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_i2)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_u2)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_i4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_u4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_i8)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_i)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_r4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_r8)
DEFINE_INSTR_NOT_IMPLEMENTED (ldind_ref)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_ref)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_i1)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_i2)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_i4)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_i8)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_r4)
DEFINE_INSTR_NOT_IMPLEMENTED (stind_r8)

DEFINE_INSTR_NOARG (add)
DEFINE_INSTR_NOARG (sub)
DEFINE_INSTR_NOARG (mul)
DEFINE_INSTR_NOARG (div)
DEFINE_INSTR_NOARG (div_un)
DEFINE_INSTR_NOARG (rem)
DEFINE_INSTR_NOARG (rem_un)
DEFINE_INSTR_NOARG (and)
DEFINE_INSTR_NOARG (or)
DEFINE_INSTR_NOARG (xor)
DEFINE_INSTR_NOARG (shl)
DEFINE_INSTR_NOARG (shr)
DEFINE_INSTR_NOARG (shr_un)
DEFINE_INSTR_NOARG (neg)
DEFINE_INSTR_NOARG (not)

#define DEFINE_INSTR_CONV(op, type) \
  void \
  instr_conv_##op () \
  { \
    scil_instr_convert (scil_##type##_type);\
  }\


DEFINE_INSTR_CONV (i1, int8)
DEFINE_INSTR_CONV (i2, int16)
DEFINE_INSTR_CONV (i4, int32)
DEFINE_INSTR_CONV (i8, int64)
DEFINE_INSTR_CONV (r4, float32)
DEFINE_INSTR_CONV (r8, float64)
DEFINE_INSTR_CONV (u4, uint32)
DEFINE_INSTR_CONV (u8, uint64)


void
instr_callvirt ()
{
  ILMethod *method;
  ILClass *owner;
  ILType *type;

  method = read_method ();
  owner = ILMethod_Owner (method);
  type = ILClassToType (owner);

  if (ILTypeIsDelegate (type))
    {
      call_delegate (owner, type, method);
    }
  else if (ILMethod_IsVirtual (method))
    {
      parse_vtable (owner);
      scil_instr_callvirt (get_tree (method), method->index);
    }
  else
    {
      scil_instr_call (get_tree (method));
    }
}


DEFINE_INSTR_NOT_IMPLEMENTED (cpobj)
DEFINE_INSTR_NOT_IMPLEMENTED (ldobj)


void
instr_ldstr ()
{
  unsigned long length;
  const char *str = 
    ILImageGetUserString (current_image, read_token() & ~IL_META_TOKEN_MASK, &length);
  scil_instr_push_string (str, (unsigned int) length);
}


void
instr_newobj ()
{
  ILMethod *ctor;
  ILClass *owner;

  ctor = read_method ();
  owner = ILMethod_Owner (ctor);

  if (ILTypeIsDelegate (ILClassToType (owner))
      && is_static_delegate (owner))
    {
      scil_instr_new_static_delegate ();
    }
  else
    {
      /* make sure the instance part of class has been parsed completely */
      parse_instance_fields (owner);
      parse_vtable (owner);
  
      scil_instr_newobj (get_tree (ctor));
    }
}


DEFINE_INSTR_NOT_IMPLEMENTED (castclass)
DEFINE_INSTR_NOT_IMPLEMENTED (isinst)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_r_un)
DEFINE_INSTR_NOT_IMPLEMENTED (unbox)
DEFINE_INSTR_NOT_IMPLEMENTED (throw)


void
instr_ldfld ()
{

  scil_instr_push_field (read_field ());
}


void
instr_ldflda ()
{
  scil_instr_push_field_address (read_field ());
}


void
instr_stfld ()
{
  scil_instr_pop_into_field (read_field ());
}


void
instr_ldsfld ()
{
  scil_instr_push (read_field ());
}


void
instr_ldsflda ()
{
  scil_instr_push_address (read_field ());
}


void
instr_stsfld ()
{
  scil_instr_pop_into (read_field ());
}

DEFINE_INSTR_NOT_IMPLEMENTED (stobj)

DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i1_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i2_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i4_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i8_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u1_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u2_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u4_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u8_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i_un)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u_un)

DEFINE_INSTR_NOT_IMPLEMENTED (box)
DEFINE_INSTR_NOT_IMPLEMENTED (newarr)
DEFINE_INSTR_NOT_IMPLEMENTED (ldlen)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelema)

DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_i1)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_u1)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_i2)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_u2)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_i4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_u4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_i8)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_i)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_r4)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_r8)
DEFINE_INSTR_NOT_IMPLEMENTED (ldelem_ref)

DEFINE_INSTR_NOT_IMPLEMENTED (stelem_i)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_i1)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_i2)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_i4)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_i8)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_r4)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_r8)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem_ref)

DEFINE_INSTR_NOT_IMPLEMENTED (ldelem)
DEFINE_INSTR_NOT_IMPLEMENTED (stelem)

DEFINE_INSTR_NOT_IMPLEMENTED (unbox_any)

DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i1)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u1)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i2)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u2)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i4)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u4)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i8)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u8)

DEFINE_INSTR_NOT_IMPLEMENTED (refanyval)
DEFINE_INSTR_NOT_IMPLEMENTED (ckfinite)
DEFINE_INSTR_NOT_IMPLEMENTED (mkrefany)

DEFINE_INSTR_NOT_IMPLEMENTED (ldtoken)

DEFINE_INSTR_CONV (u2, uint16)
DEFINE_INSTR_CONV (u1, uint8)
DEFINE_INSTR_CONV (i, native_int)

DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_i)
DEFINE_INSTR_NOT_IMPLEMENTED (conv_ovf_u)

DEFINE_INSTR_NOT_IMPLEMENTED (add_ovf)
DEFINE_INSTR_NOT_IMPLEMENTED (add_ovf_un)
DEFINE_INSTR_NOT_IMPLEMENTED (mul_ovf)
DEFINE_INSTR_NOT_IMPLEMENTED (mul_ovf_un)
DEFINE_INSTR_NOT_IMPLEMENTED (sub_ovf)
DEFINE_INSTR_NOT_IMPLEMENTED (sub_ovf_un)

DEFINE_INSTR_NOT_IMPLEMENTED (endfinally)
DEFINE_INSTR_NOT_IMPLEMENTED (leave)
DEFINE_INSTR_NOT_IMPLEMENTED (leave_s)

DEFINE_INSTR_NOT_IMPLEMENTED (stind_i)
DEFINE_INSTR_CONV (u, native_uint)


void
instr_extended ()
{
# ifdef SCIL_TRACE_VIRTUAL_EXECUTION
    fnotice (stderr, "execute 0x%x: %s\n", pc, extended_instruction_names [code [pc]]);
# endif

  extended_instructions [code[pc++]] ();
}


/* extended instructions */

#define DEFINE_EXTD_INSTR_NOT_IMPLEMENTED(name) \
  void \
  instr_##name () \
  { \
    sorry (extended_instruction_names [code [pc - 1]]); \
  }


DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (arglist)


void
instr_ceq ()
{
  scil_instr_compare_eq (); 
}


void
instr_cgt ()
{
  scil_instr_compare_gt (); 
}


void
instr_cgt_un ()
{
  scil_instr_compare_gt_un ();
}


void
instr_clt ()
{
  scil_instr_compare_lt (); 
}


void
instr_clt_un ()
{
  scil_instr_compare_lt_un ();
}

void
instr_ldftn ()
{
  scil_instr_push_address (get_tree (read_method ()));
}

void
instr_ldvirtftn ()
{
  ILMethod *method;

  method = read_method ();
  parse_vtable (ILMethod_Owner (method));

  scil_instr_push_virtual_method (method->index);
}

void
instr_ldarg ()
{
  scil_instr_push (parameters[read_uint16 ()]);
}


void
instr_ldarga ()
{
  scil_instr_push_address (parameters[read_uint16 ()]);
}


void
instr_starg ()
{
  scil_instr_pop_into (parameters[read_uint16 ()]);
}


void
instr_ldloc ()
{
  scil_instr_push (locals[read_uint16 ()]);
}


void
instr_ldloca ()
{
  scil_instr_push_address (locals[read_uint16 ()]);
}


void
instr_stloc ()
{
  scil_instr_pop_into (locals[read_uint16 ()]);
}


DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (localloc)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (endfilter)

DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (unaligned_)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (volatile_)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (tail_)

DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (initobj)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (cpblk)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (initblk)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (rethrow)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (sizeof)
DEFINE_EXTD_INSTR_NOT_IMPLEMENTED (refanytype)


void
instr_invalid ()
{
  error ("Invalid instruction code %x", code[pc-1]);
}
