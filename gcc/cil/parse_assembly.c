/* cil-decl.h include: config.h system.h tree.h */
#include "cil-decl.h"

/* GCC headers */
#include "toplev.h"
#include "coretypes.h"
#include "real.h"
#include "cgraph.h"

#include "build_trees.h"
#include "mono_structs.h"
#include "parse_assembly.h"
#include "parse_instr.h"
#include "pnet.h"
#include "mangle.h"

/* ILCONTEXT for assemblies and modules */
static ILContext *context;

/* ILIMAGE (assembly or module), which is currently parsed */
ILImage *c_image;

static ILToken c_entry_point;

static tree add_vtable_slot (tree entries, ILMethod* method);
static tree change_vtable_slot (tree entries, ILMethod* method);
static void set_method_indices_to_invalid (ILClass* klass);
static void create_intf_methods_indices (ILClass* klass);
static unsigned int create_virtual_methods_indices (ILClass* klass);
static unsigned int create_virtual_methods_indices_for_interfaces (ILClass* klass, unsigned int index);
static tree create_vtable_entries (ILClass *klass);
static tree get_array_type (tree elem_type);
static tree get_assembly_name (ILClass *klass);
static tree get_class_ptr_type (ILClass* klass);
static tree get_value_type (ILClass* klass);
static tree get_function_ptr (ILMethod *method);
static tree get_static_fields_decl (ILClass* klass);
static char is_entry_point (ILMethod *method);
static ILClass* parent (ILClass *klass);
static void parse_all_methods (void);
static tree parse_constant (ILConstant *constant);
static tree parse_class (ILClass *klass);
static void parse_classes (void);
static void parse_field (ILClass* context_class, tree record, ILField *il_field);
static void parse_fields (ILClass* klass, tree record);
static void parse_static_fields (ILClass* klass);
static void parse_locals (ILMethodCode *code);
static void parse_method (ILToken token);
static void parse_method_body (ILMethod *method);
static tree parse_parameters (ILMethod *method);
static void parse_runtime_method_body (ILMethod *method);

void
parse_assembly (const char *filename)
{
  context = ILContextCreate ();
 	
  load_mscorlib (context);

  load_assembly (context, &c_image, filename);
	
  c_entry_point = ILImageGetEntryPoint (c_image);
	
  parse_classes ();

  parse_all_methods ();
	
  ILContextDestroy (context);

  /* Perform unit-at-a-time compilation */

  cgraph_finalize_compilation_unit ();

  cgraph_optimize ();
}

static void
parse_classes (void)
{
  unsigned long token;
  unsigned long numTokens;

  numTokens = ILImageNumTokens (c_image, IL_META_TOKEN_TYPE_DEF);

  /* Create RECORD_TYPES for all classes */
  for (token = 1; token <= numTokens; ++token)
  {
		ILClass *klass = ILClass_FromToken (c_image,
					  IL_META_TOKEN_TYPE_DEF | token);

		/* if type def is not a "<Module>" */
		if (strcmp (ILClass_Name (klass), "<Module>") != 0)
		{
			parse_class (klass);
		}
	}
}

static ILClass*
parent (ILClass *klass)
{
	ILClass *result;

	result = ILClassGetParent (klass);

	if (result && ILClassIsRef (result)) {
		result = resolve_class_ref (context, result);
	}
	
	return result;
}

/* function is called for type defs and type refs 
 * if class is in assembly and not a interface this function
 * will return a pointer to a tree node representing the created 
 * identifier for class (class_name)
 * otherwise a null-pointer.
 */
static tree
parse_class (ILClass *klass)
{
  ILClass *base_klass, *intf_klass;
  ILImplements *implement;
  
  /* to iterate over and count all defined methods */
  int method_count, instance_size, vtable_size;
  ILMember *member;
  
  /*
   * VTABLE_ENTRIES: - is a tree list
   * 		   - purpose: index
   * 		   - value: function pointer
   */
  tree vtable_entries, vt_name;
  tree class_decl, class_name, mono_class_name;
  tree parent_mono_class_name, mono_class_runtime_info_name;
  
  /* collect decls of mono_classes (mono_class_name) for all interfaces
	 * this class implements (for each in implements clause) */
  tree intf_mono_class_name, list_node, intf_mono_class_name_list;
  

	mono_class_name = mangle_mono_class(klass);
	
	/* check if this class is already made 
	 * e.g. because it is base type of another class already parsed */
	if (! IDENTIFIER_DECL (mono_class_name)) {
  	 
    /* if class is in assembly */
    if (ILClassToImage (klass) == c_image)
    {
		  parent_mono_class_name = 0;
      intf_mono_class_name_list = NULL_TREE;
      
      
      /* ensure that all entries in the implements clause are parsed (for interfaces and classes)
       * because a MonoClass struct references all implemented interfaces 
       * If the parsed class is an interface the implements clause can contain
       * further interfaces that form base interfaces of the parsed one */
       
	    implement = 0;
      implement = ILClassNextImplements (klass, implement);
      
      while (implement) 
      {
        intf_klass = ILImplementsGetInterface (implement);
        
        intf_mono_class_name = parse_class (intf_klass);
          
        if (intf_mono_class_name) 
        {
          list_node = build_tree_list (0, intf_mono_class_name);
          intf_mono_class_name_list = chainon ( intf_mono_class_name_list,
                                             list_node);
        }       
          
        /* iterate, fetch next in list */
        implement = ILClassNextImplements (klass, implement);
      }
		  

      /* ensure that base klass is parsed
       * because a MonoClass struct has a reference to it (parent) */
		  if (! ILClass_IsInterface (klass)) 
      {  
        base_klass = parent (klass);
      
        if (base_klass) 
        {
          parent_mono_class_name = parse_class (base_klass);
        }	
		  }
    }
  
  		
		
	  class_name = get_namespace_class_name (klass);
	
	  class_decl = create_class_decl (klass, class_name);
	  
	  parse_fields (klass, class_decl);
	
	  parse_static_fields (klass);
		
	  finish_class (class_name);
	  
		
	  /* if class is in assembly we have to compile,
	   * and is class not an interface then create MonoVTable */
	  if (ILClassToImage (klass) == c_image)
	  {
      /* iterate over and count methods */
      member = 0;
      method_count = 0;
      member = ILClassNextMemberByKind (klass, member, IL_META_MEMBERKIND_METHOD);
      while (member) {
        method_count++;
        member = ILClassNextMemberByKind (klass, member, IL_META_MEMBERKIND_METHOD);
      }
      
      if (ILClass_IsInterface (klass)) 
      {
        make_intf_mono_class (mono_class_name, intf_mono_class_name_list, method_count);
        create_intf_methods_indices(klass);
      }
      else 
      {
        /* create only decl for mono_class first
         * because mono_vtable references it */
        create_mono_class_decl (mono_class_name);
        									
        
        /* create vtable for mono class 
         * ensure that MonoClass is created first 
         * because MonoVtable reference MonoClass */
        create_virtual_methods_indices (klass);
        
        vtable_entries = create_vtable_entries (klass);
      	
        	vt_name = mangle_vtable (klass);
        	
        	/* TODO: replace class_name with mono_class_name */
        	make_mono_vtable (class_name, vt_name, vtable_entries);
      
        
        mono_class_runtime_info_name = mangle_mono_class_runtime_info (klass);
        make_mono_class_runtime_info (mono_class_runtime_info_name, vt_name);
        
        instance_size = TREE_INT_CST_LOW (TYPE_SIZE_UNIT (class_decl));
        vtable_size = list_length (vtable_entries);
                 
        /* mono_vtable must be created first
         * because it is referenced from runtime_info in mono_class */
        make_mono_class ( mono_class_name, intf_mono_class_name_list,
                          parent_mono_class_name, method_count, instance_size,
                          vtable_size, mono_class_runtime_info_name );
      }
	  } 
	  else 
	  {
	  		/* create a external declaration of mono_class */
	  		make_ext_mono_class (mono_class_name);
	  }
		
	  if (ILClassIsValueType (klass))
	  {
			tree value = NULL_TREE;
			if (!strcmp (ILClass_Namespace (klass), "System")
					&& ILClass_NestedParent (klass) == NULL)
			{
				if (!strcmp (ILClass_Name (klass), "SByte"))
			    value = int8_type_node;
			  else if (!strcmp (ILClass_Name (klass), "Byte"))
			    value = uint8_type_node;
			  if (!strcmp (ILClass_Name (klass), "Int8"))
			    value = int8_type_node;
			  else if (!strcmp (ILClass_Name (klass), "UInt8"))
			    value = uint8_type_node;
			  if (!strcmp (ILClass_Name (klass), "Int16"))
			    value = int16_type_node;
			  else if (!strcmp (ILClass_Name (klass), "UInt16"))
			    value = uint16_type_node;
			  if (!strcmp (ILClass_Name (klass), "Int32"))
			    value = int32_type_node;
			  else if (!strcmp (ILClass_Name (klass), "UInt32"))
			    value = uint32_type_node;
			  if (!strcmp (ILClass_Name (klass), "Int64"))
			    value = int64_type_node;
			  else if (!strcmp (ILClass_Name (klass), "UInt64"))
			    value = uint64_type_node;
			  else if (!strcmp (ILClass_Name (klass), "Single"))
			    value = float_type_node;
			  else if (!strcmp (ILClass_Name (klass), "Double"))
			    value = double_type_node;
			  else if (!strcmp (ILClass_Name (klass), "Char"))
			    value = char_type_node;
			  else if (!strcmp (ILClass_Name (klass), "Boolean"))
			    value = boolean_type_node;
			  else if (!strcmp (ILClass_Name (klass), "Void"))
			    value = void_type_node;
			}
	    
	    if (!value) 
			{
		  		value = create_value_decl ();
		  		parse_fields (klass, value);
		  		layout_type (value);
			}
	      TYPE_VALUETYPE (class_decl) = value;
	      TYPE_OBJECTTYPE (value) = class_decl;
	    }
	    
	}
  
	return mono_class_name;
}

tree
get_class_decl (ILClass* klass)
{
  tree class_name = get_namespace_class_name (klass);

  tree class_decl = IDENTIFIER_CLASS_DECL (class_name);

  if (class_decl == NULL_TREE)
    {
      /* class is defined in external assembly */
      if (ILClassIsRef (klass))
	{
	  klass = resolve_class_ref (context, klass);

	  class_decl = create_class_decl (klass, class_name);

	  parse_class (klass);
	}
      else
	{
	  class_decl = create_class_decl (klass, class_name);
	}
    }

  return class_decl;
}

tree
get_class_ptr_type (ILClass* klass)
{
  return build_pointer_type (get_class_decl (klass));
}

tree
get_value_type (ILClass *type)
{
  tree klass = get_class_decl (type);
  if (TYPE_VALUETYPE (klass) == NULL)
    {
      /* XXX generates methods multiple times for external structs? */
      parse_class (type);
    }
  return TYPE_VALUETYPE (klass);
}

tree
get_static_fields_decl (ILClass* klass)
{
  tree class_name = get_namespace_class_name (klass);

  tree s_fields_decl = IDENTIFIER_STATIC_FIELDS_DECL (class_name);
	
  if (s_fields_decl == NULL_TREE)
    {
      s_fields_decl = create_static_fields_decl (klass, class_name);
    }
	
  return s_fields_decl;
}


/* adds fields of KLASS to class_decl */
void
parse_fields (ILClass *klass, tree class_decl)
{
	ILClass *base_klass;
	ILMember *member;
	
	
	base_klass = parent (klass);
	
	if (base_klass) 
	{
		parse_fields (base_klass, class_decl);
	}
	
	
  member = 0;

  while ((member = ILClassNextMember (klass, member)) != 0)
  {
		if (ILMember_IsField (member) && !ILField_IsStatic (member))
		{
	  		parse_field (klass, class_decl, (ILField *)member);
		}
	}
}

void
parse_static_fields (ILClass *klass)
{
  ILMember *member = 0;
  tree statics = get_static_fields_decl (klass);

  while ((member = ILClassNextMember (klass, member)) != 0)
    if (ILMember_IsField (member) && ILField_IsStatic (member))
      {
	ILField *il_field = (ILField*)member;
	tree field;
	ILType *il_type = ILFieldGetType (il_field);
	tree type =  parse_type (il_type);
	ILConstant *constant = ILConstantGetFromOwner (ILToProgramItem (il_field));

	field = add_field (statics, type, NULL_TREE, 
			   get_identifier (ILField_Name (il_field)));

	if (constant)
	  DECL_INITIAL (field) = parse_constant (constant);
      }
}
      

/* add a field of KLASS to TO_CLASS */
void
parse_field (ILClass* context_class, tree record, ILField *il_field)
{
  ILType *il_type = ILFieldGetType (il_field);
  tree type =  parse_type (il_type);
  tree to_type = get_class_decl (context_class);
	
  tree field;

  field = add_field (record, type, to_type, 
		     get_identifier (ILField_Name (il_field)));
}


void 
set_method_indices_to_invalid(ILClass* klass)
{
    ILMethod* method;
    
    method = (ILMethod*) ILClassNextMemberByKind(klass, 0, IL_META_MEMBERKIND_METHOD);
    while (method)
    {
        method->count = 0;
        method = (ILMethod*) ILClassNextMemberByKind(klass, (ILMember*) method, IL_META_MEMBERKIND_METHOD);
    }
}

void
create_intf_methods_indices (ILClass* klass)
{
	ILMethod* method;
  int index = 0;
  
  method = (ILMethod*) ILClassNextMemberByKind(klass, 0, IL_META_MEMBERKIND_METHOD);
  while (method)
  {
      method->index = index++;
      method = (ILMethod*) ILClassNextMemberByKind(klass, (ILMember*) method, IL_META_MEMBERKIND_METHOD);
  }
	
}

unsigned int
create_virtual_methods_indices (ILClass* klass)
{
  ILMethod *method, *parent_method;
  unsigned int index = 0;
 
  if (ILClassGetParent (klass))
  {
    index = create_virtual_methods_indices (ILClassGetParent (klass));
  }
    
	
  index = create_virtual_methods_indices_for_interfaces (klass, index);

  method = (ILMethod*) ILClassNextMemberByKind(klass, 0, IL_META_MEMBERKIND_METHOD);
  while (method)
  {
    if (ILMethod_IsVirtual (method) && !ILMethod_IsRuntime (method) && !method->count)
    {
      parent_method = (ILMethod*) ILMemberGetBase ((ILMember*) method);
  
      method->index = parent_method ? parent_method->index : index++;
      method->count = 1;    
    }
    method = (ILMethod*) ILClassNextMemberByKind(klass, (ILMember*) method, IL_META_MEMBERKIND_METHOD);
  }
 
  set_method_indices_to_invalid(klass);
  return index;
}



unsigned int
create_virtual_methods_indices_for_interfaces (ILClass* klass, unsigned int index)
{
  ILImplements* impl;
  ILClass* interface;
  ILMethod* method;
  ILMethod* implementation_method;
  
  impl = ILClassNextImplements (klass, 0);
  while (impl)
  {
    interface = ILImplementsGetInterface (impl);
    
    method = (ILMethod*) ILClassNextMemberByKind(interface, 0, IL_META_MEMBERKIND_METHOD);
    while (method)
    {
        /* TODO: ILClassGetMethodImpl also searches for the implementation in 
         * all parent classes. This may cause errors with the indices */
        implementation_method = ILClassGetMethodImpl (klass, method);
	if (!implementation_method)
	{
		internal_error("%s does not implement %s\n", ILClass_Name(klass),
			ILMethod_Name(method));
	}
       	implementation_method->index = index++;
       	implementation_method->count = 1;
        method = (ILMethod*) ILClassNextMemberByKind(interface, (ILMember*) method, IL_META_MEMBERKIND_METHOD);
    }
    impl = ILClassNextImplements(klass, impl);
  }
  return index;
}
  



tree
create_vtable_entries (ILClass *klass)
{
  ILMember *member;
  tree entries;

  if (klass == 0)
  {
    return 0;
  }

  entries = create_vtable_entries (ILClassGetParent (klass));

  member = 0;
  while ((member = ILClassNextMember (klass, member)) != 0)
  {
    ILMethod* method;

    if (!ILMember_IsMethod (member))
    {
      continue;
    }

    method = (ILMethod*) member;

    if (ILMethod_IsVirtual (method) && !ILMethod_IsRuntime (method))
    	{
      ILMethod* parent_method = (ILMethod*) ILMemberGetBase (member);

      entries = parent_method ? 
                change_vtable_slot (entries, method) :
                add_vtable_slot (entries, method);
    	}
  }

  return entries;
}

tree
add_vtable_slot (tree entries, ILMethod* method)
{
  tree result, new_entry, entry_before_new;
  
  new_entry = build_tree_list (build_int_cst 
                (integer_type_node, method->index),
                get_function_ptr (method));

  if (! entries)
  {
    result = new_entry;
  }
  else 
  {
    entry_before_new = entries;
    while ( TREE_CHAIN (entry_before_new) )
    {
      if (TREE_INT_CST_LOW (TREE_PURPOSE ( TREE_CHAIN (entry_before_new))) > method->index)
      {
        break;
      }
    
      if (TREE_INT_CST_LOW (TREE_PURPOSE ( TREE_CHAIN (entry_before_new))) == method->index)
      {
        internal_error ("add_vtable_slot (): a vtable entry with same index already exist");
      }
      
      entry_before_new = TREE_CHAIN (entry_before_new);
    }
    
    if (TREE_CHAIN (entry_before_new))
    {
      TREE_CHAIN (new_entry) = TREE_CHAIN (entry_before_new);
    }
    
    TREE_CHAIN (entry_before_new) = new_entry;
    
    result = entries;
  }

  return result;
}

tree
change_vtable_slot (tree entries, ILMethod* method)
{
  unsigned int i = 0;

  tree entry = entries;
	
  while (i < method->index)
    {
      entry = TREE_CHAIN (entry);
		
      if (!entry)
	{
	  internal_error ("change_vtable_slot (): invalid index");
	}
		
      ++i;
    }
	
  TREE_VALUE (entry) = get_function_ptr (method);

  return entries;
}

tree
get_function_ptr (ILMethod *method)
{
  tree method_name, param_type_list, return_type;
         
  method_name = mangle_function (method);
  param_type_list = parse_parameters_types (method);
  return_type = parse_return_type (method);
	
  return create_function_ptr (method_name, param_type_list, return_type);
}

void
parse_all_methods (void)
{
  unsigned long token;
  unsigned long numTokens;

  numTokens = ILImageNumTokens (c_image, IL_META_TOKEN_METHOD_DEF);

  for (token = 1; token <= numTokens; ++token)
    {
      parse_method (IL_META_TOKEN_METHOD_DEF | token);
    }
}

char
is_entry_point (ILMethod *method)
{

  return (ILMethod_Token (method) == c_entry_point);
}

void
parse_method (ILToken token)
{
  ILMethod *method;
  tree method_name, return_type, params, method_decl;
        
  method = ILMethod_FromToken (c_image, token);

  if (!method)
  {
    internal_error ("Error while building method");
  }
	
  if (ILClass_IsInterface (ILMethod_Owner (method)))
  {
    return;
  }

  if (!method->rva && !ILMethod_IsRuntime (method))
  /* csc has a bug: it generates non-abstract, non-runtime methods with RVA=0.
     we use that for external methods. */
  {
    /* TODO: consider attributes, to change assembler name */
    return;
  }

  method_name = mangle_function (method);

  params = parse_parameters (method);

  return_type = parse_return_type (method);

  method_decl = make_current_function_decl (method_name,
					    return_type,
					    params);

  if (is_entry_point (method))
  {
    SET_DECL_ASSEMBLER_NAME (method_decl, get_identifier ("main"));
  }

  if (ILMethod_IsStaticConstructor (method))
  {
    if (!ILClass_IsBeforeFieldInit (ILMethod_Owner (method)))
    	{
    	  internal_error ("cctor, whose corresponding class is not"
    			  " 'beforefieldinit', is not supported yet");
    	}

    DECL_STATIC_CONSTRUCTOR (method_decl) = 1;
  }
	
  if (ILMethod_IsRuntime (method))
  {
    parse_runtime_method_body (method);
  }
  else
  {
    parse_method_body (method);
  }

  compile_function ();
}

void
parse_method_body (ILMethod *method)
{
  ILMethodCode method_code;

  /* Read the method code */
  if (!ILMethodGetCode (method, &method_code))
    {
      /* If we get here, then probably the method had an RVA,
	 but the code was not IL */
      internal_error ("Method has no IL code (RVA %x)", method->rva);
    }

  parse_locals (&method_code);

  parse_instructions (&method_code);
}

void
parse_runtime_method_body (ILMethod *method)
{
  const char *name;
	
  ILClass *klass = ILMethod_Owner (method);
	
  ILType *type = ILType_FromClass (klass);
	
  if (!ILTypeIsDelegate (type))
    {
      internal_error ("runtime method not supported yet");
    }
	
  name = ILMethod_Name (method);

  if (!strcmp (name, ".ctor"))
    {
      make_delegate_ctor ();
      return;
    }
  else if (!strcmp (name, "Invoke"))
    {
      make_delegate_invoke ();
      return;
    }
  else if (!strcmp (name, "BeginInvoke"))
    {
      warning0 ("runtime method 'BeginInvoke' of a delegate ignored"
		" - not supported yet");
			 
      return;
    }
  else if (!strcmp (name, "EndInvoke"))
    {
      warning0 ("runtime method 'EndInvoke' of a delegate ignored"
		" - not supported yet");
      return;
    }
	
  internal_error ("unknown runtime method");
}

tree
parse_parameters (ILMethod *method)
{
  tree params = NULL_TREE;

  ILParameter *param = 0;

  ILType *sig = ILMethod_Signature (method);
	
  if (!ILMethod_IsStatic (method))
    {
      ILClass *klass = ILMethod_Owner (method);

      tree obj_ref_type = get_class_ptr_type (klass);
		
      tree name = get_identifier ("this_ptr");

      params = build_tree_list (obj_ref_type, name);
    }
	
  while ((param = ILMethodNextParam (method, param)) != 0 )
    {
      ILType *il_type = ILTypeGetParam (sig,
					ILParameterGetNum (param));
      tree type = parse_type (il_type);

      tree name = get_identifier (ILParameterGetName (param));
		
      params = chainon (params, build_tree_list (type, name));
    }
	
  return params;
}

tree
parse_parameters_types (ILMethod *method)
{
  tree params = NULL_TREE;
  unsigned long num_params, param;
	
  ILType *sig = ILMethod_Signature (method);

  if (ILType_HasThis (sig))
    {
      ILClass *klass = ILMethod_Owner (method);

      tree obj_ref_type = get_class_ptr_type (klass);

      params = build_tree_list (obj_ref_type, NULL_TREE);
    }

  num_params = ILTypeNumParams (sig);

  for (param = 1; param <= num_params; ++param)
    {
      ILType *il_type = ILTypeGetParam (sig, param);

      tree type = parse_type (il_type);

      params = chainon (params, build_tree_list (type, NULL_TREE));
    }
	
  return params;
}

tree
parse_return_type (ILMethod *method)
{
  ILType *sig = ILMethod_Signature (method);

  ILType *type = ILTypeGetReturn (sig);
	
  return parse_type (type);
}

void
parse_locals (ILMethodCode* code)
{
  ILType *locals;
  unsigned long num;
  unsigned long index;

  tree local_vars = NULL_TREE;

  if (!code->localVarSig)
    {
      return;
    }

  locals = ILStandAloneSigGetType (code->localVarSig);

  num = ILTypeNumLocals (locals);

  for (index = 0; index < num; ++index)
    {
      ILType *il_type = ILTypeGetLocal (locals, index);

      tree type = parse_type (il_type);
		
      tree name;
      char *buf = alloca (10);
      sprintf (buf, "LOC_%ld", index);
      name = get_identifier (buf);
		
      local_vars = chainon (local_vars, build_tree_list (type, name));
    }
	
  add_local_vars (local_vars);
}

tree
get_assembly_name (ILClass *klass)
{
  if (!ILClassIsRef (klass))
    {
      const char *name = ILImageGetAssemblyName (
						 ILClassToImage (klass));
      return get_identifier (name);

    }
  else
    {
      return get_identifier (get_assembly_ref_name (klass));
    }
}

tree
get_namespace_class_name (ILClass *klass)
{
  tree name;
  if (ILClass_Namespace (klass))
    {
      name = make_complex_identifier (
				      get_identifier (ILClass_Namespace (klass)),
				      get_identifier (ILClass_Name (klass)),
				      NULL_TREE);
    }
  else if (ILClass_IsNestedPublic (klass)
	   || ILClass_IsNestedPrivate (klass)
	   || ILClass_IsNestedFamily (klass))
    {
      ILClass *parent = ILClass_NestedParent (klass);

      if (ILClass_Namespace (parent))
	{
	  name = make_complex_identifier 
	    (get_identifier (ILClass_Namespace (parent)),
	     get_identifier (ILClass_Name (parent)),
	     get_identifier (ILClass_Name (klass)),
	     NULL_TREE);
	}
      else
	{
	  name = make_complex_identifier 
	    (get_identifier (ILClass_Name (parent)),
	     get_identifier (ILClass_Name (klass)),
	     NULL_TREE);
	}
    }
  else
    {
      name = get_identifier (ILClass_Name (klass));
    }
	
  name = make_complex_identifier (
				  get_assembly_name (klass),
				  name,
				  NULL_TREE);
  return name;
}

tree
parse_type (ILType *il_type)
{
  if (ILType_IsPrimitive (il_type))
    {
      switch (ILType_ToElement (il_type))
	{
	case IL_META_ELEMTYPE_VOID:
	  {
	    return void_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_BOOLEAN:
	  {
	    return boolean_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_CHAR:
	  {
	    return cil_char_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_I1:
	  {
	    return int8_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_U1:
	  {
	    return uint8_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_I2:
	  {
	    return int16_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_U2:
	  {
	    return uint16_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_I4:
	  {
	    return int32_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_U4:
	  {
	    return uint32_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_I8:
	  {
	    return int64_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_U8:
	  {
	    return int64_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_R4:
	  {
	    return float_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_R8:
	  {
	    return double_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_TYPEDBYREF:
	  {
	    internal_error ("Type not implemented");
	  }
	  break;

	case IL_META_ELEMTYPE_I:
	  {
	    return native_int_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_U:
	  {
	    return native_uint_type_node;
	  }
	  break;

	case IL_META_ELEMTYPE_R:
	  {
	    internal_error ("Type not implemented");
	  }
	  break;

	case IL_META_ELEMTYPE_SENTINEL:
	  {
	    internal_error ("Type not implemented");
	  }
	  break;

	default:
	  {
	    internal_error ("Type not implemented");
	  }
	  break;
	}

    }
  else if (ILType_IsClass (il_type))
    {
      ILClass *klass = ILType_ToClass (il_type);

      return get_class_ptr_type (klass);
    }
  else if (ILType_IsValueType (il_type))
    {
      ILClass* val_type = ILType_ToValueType (il_type);

      if (ILClassIsRef (val_type))
        {
          val_type = resolve_class_ref (context, val_type);
          il_type = ILType_FromValueType (val_type);
        }

      if (ILTypeIsEnum (il_type))
	{
	  return parse_type (ILTypeGetEnumType (il_type));
	}

      return get_value_type (val_type);
    }

  else if (ILType_IsSimpleArray (il_type))
    {
      return get_array_type (parse_type (ILTypeGetElemType (il_type)));
    }
  else if (ILType_IsArray (il_type))
    {
      /* TODO: support complex arrays */
      internal_error ("complex array is not supported - "
		      "used void* instead\n");
    }
  else if (ILType_Kind (il_type) == IL_TYPE_COMPLEX_BYREF) 
    {
      return build_pointer_type (parse_type (ILType_Ref (il_type)));
    }
  else if (ILType_IsPointer (il_type))
    {
      return build_pointer_type (parse_type (ILType_Ref (il_type)));
    }
  else
    {
      internal_error ("Type not implemented (%d)", ILType_Kind (il_type));
    }

}

tree
get_array_type (tree elem_type)
{
  tree array_decl = TYPE_ARRAYTYPE (elem_type);

  if (array_decl == NULL_TREE)
    {
      array_decl = create_array_decl (elem_type);
      TYPE_ARRAYTYPE (elem_type) = array_decl;
                
    }
  return build_pointer_type (array_decl);
}

tree
parse_constant (ILConstant *constant)
{
  const unsigned char *blob;
  unsigned long blobLen;

  /* Get the blob that corresponds to the constant value */
  blob = (const unsigned char *) ILConstantGetValue (constant, &blobLen);

  if (!blob)
    {
      /* Treat non-existent blobs as empty: sometimes empty
	 strings are encoded as a constant with no blob */
      blobLen = 0;
    }

  /* return expression based on its element type */

  switch (ILConstant_ElemType (constant))
    {
    case IL_META_ELEMTYPE_BOOLEAN:
      {
	if (blobLen > 0)
	  {
	    if (blob[0])
	      {
		return boolean_true_node;
	      }
	    else
	      {
		return boolean_false_node;
	      }
	  }
	else
	  {
	    internal_error ("invalid boolean constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_I1:
      {
	if (blobLen > 0)
	  {
	    char num = (char)(blob[0]);
	    tree t = build_int_cst (int8_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid int8 constant");
	  }
      }
      break;
		
    case IL_META_ELEMTYPE_U1:
      {
	if (blobLen > 0)
	  {
	    unsigned char num = (unsigned char)(blob[0]);
	    tree t = build_int_cst (uint8_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid uint8 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_CHAR:
      {
	if (blobLen > 1)
	  {
	    ILUInt16 num = IL_READ_UINT16 (blob);
	    tree t = build_int_cst (cil_char_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid char constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_I2:
      {
	if (blobLen > 1)
	  {
	    ILInt16 num = IL_READ_INT16 (blob);
	    tree t = build_int_cst (int16_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid int16 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_U2:
      {
	if (blobLen > 1)
	  {
	    ILUInt16 num = (int) IL_READ_UINT16 (blob);
	    tree t = build_int_cstu (uint16_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid uint16 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_I4:
      {
	if (blobLen > 1)
	  {
	    ILInt32 num = IL_READ_INT32 (blob);
	    tree t = build_int_cst (int32_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid int32 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_U4:
      {
	if (blobLen > 1)
	  {
	    ILUInt32 num = IL_READ_UINT32 (blob);
	    tree t = build_int_cstu (uint32_type_node, num);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid uint32 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_I8:
      {
	if (blobLen > 1)
	  {
	    ILInt64 num = IL_READ_INT64 (blob);
	    tree t = build_int_cst_wide (int64_type_node,
					 num & 0xFFFFFFFFU,
					 num >> 32);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid uint32 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_U8:
      {
	if (blobLen > 1)
	  {
	    ILUInt64 num = IL_READ_UINT64 (blob);
	    tree t = build_int_cst_wide (uint64_type_node,
					 num & 0xFFFFFFFFU,
					 num >> 32);
	    return t;
	  }
	else
	  {
	    internal_error ("invalid uint32 constant");
	  }
      }
      break;

    case IL_META_ELEMTYPE_R4:
      {
	if (blobLen >= 4)
	  {
	    ILInt32 num = IL_READ_INT32 (blob);
	    REAL_VALUE_TYPE val;
	    tree t;

	    real_from_target_fmt (&val, (long*)&num, &ieee_single_format);
	    t = build_real (float_type_node, val);
	    return t;
	  }
	else
	  internal_error ("invalid float constant");
      }
      break;
				
    case IL_META_ELEMTYPE_R8:
      {
	if (blobLen >= 8)
	  {
	    ILInt64 num = IL_READ_INT64 (blob);
	    REAL_VALUE_TYPE val;
	    tree t;

	    real_from_target_fmt (&val, (long*)&num, &ieee_double_format);
	    t = build_real (double_type_node, val);
	    return t;
	  }
	else
	  internal_error ("invalid float constant");
      }
      break;

      /* TODO: implement all possible types of a constant */
    case IL_META_ELEMTYPE_STRING:
    case IL_META_ELEMTYPE_CLASS:
    default:
      {
	internal_error ("parse_constant(): type of constant "
			"not implemented yet");
      }
      break;
    }
}

