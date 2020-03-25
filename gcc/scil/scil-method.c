/* The CIL front end callbacks for building classes. 
 */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "tree-iterator.h"
#include "tree-gimple.h"
#include "tree-dump.h"
#include "cgraph.h"

#include "ggc.h"
#include "toplev.h"

#include "scil-config.h"
#include "scil-hooks.h"
#include "scil-callbacks.h"
#include "scil-rt.h"
#include "scil-trees.h"
#include "scil-builtins.h"
#include "scil-mangle.h"
#include "scil-evalstack.h"

static tree current_param_type;
static bool types_equal_p (tree, tree);

/* Returns true if the TYPE1 is the same as TYPE2, ie if the argument types of
   both types and their result types are the same.
*/
bool
types_equal_p (tree type1, tree type2)
{
  tree t1, t2;

  if (type1 == type2)
    return true;

  if (TREE_TYPE (type1) != TREE_TYPE (type2))    
      return false;

  
  for (t1 = TYPE_ARG_TYPES (type1), t2 = TYPE_ARG_TYPES (type2); 
       t1 != NULL_TREE && t2 != NULL_TREE; 
       t1 = TREE_CHAIN(t1), t2 = TREE_CHAIN(t2))
    {
      /* reference equality here because we look for the same, not equal type */
      if (TREE_VALUE(t1) != TREE_VALUE(t2))
        return false;
    }        
  
  /* if one type has more arguments than the other */
  if (t1 != NULL_TREE || t2 != NULL_TREE)
    return false;
  
  return true;
}

/* see scil-callbacks.h */
/* For each method we build a TREE_LIST node containing the FUNCTION_DECL node
   in order to cope with overloaded method names.
*/
tree
scil_register_method_impl (tree container, 
                           const char *method_name, 
                           tree method_type, 
                           int runtime, 
                           int mangle_name)
{
  tree id, decl;

  id = container != NULL_TREE ? 
       scil_get_nested_identifier (TYPE_NAME (container), method_name):
       get_identifier (method_name);

  /* If we already have that method (eg because the runtime registered it 
     before) return that one. */
  tree t;
  for (t = SCIL_DECL (id); t != NULL_TREE; t = TREE_CHAIN (t))
    {
      if (types_equal_p (TREE_TYPE (TREE_VALUE (t)), method_type))
        return TREE_VALUE (t);
    }

  /* build method decl */
  decl = scil_build_decl (FUNCTION_DECL, id, method_type);
  DECL_RESULT (decl) = scil_build_decl (RESULT_DECL, NULL_TREE, TREE_TYPE (method_type));
  DECL_CONTEXT (decl) = container;
  allocate_struct_function (decl);

  DECL_EXTERNAL (decl) = runtime;
  
  /* mangle name */
  SCIL_DECL_UNQUALIFIED (decl) = get_identifier (method_name);
  if (mangle_name)
    {      
      SET_DECL_ASSEMBLER_NAME (decl, scil_mangle_method (decl));
    }
  
  /* add new method decl to the end of the overload list */
  SCIL_DECL (id) = tree_cons (NULL_TREE, decl, SCIL_DECL (id));
 
  return decl;
}


/* see scil-callbacks.h */
/* We use TREE_TYPE of the method type as the result declaration, and 
   TYPE_ARG_TYPES as the list of parameter declarations */
tree
scil_make_method_type (tree return_type)
{
  tree method_type;

  method_type = make_node (FUNCTION_TYPE);

  TYPE_LANG_SPECIFIC (method_type) = GGC_CNEW (struct lang_type);
  TREE_TYPE (method_type) = return_type;
  TYPE_ARG_TYPES (method_type) = NULL_TREE;

  return method_type;
}


/* see scil-callbacks.h */
void 
scil_add_method_parameter_type (tree method_type, tree type, bool byref)
{
  tree actual_type;

  actual_type = byref ? build_pointer_type (type) : type;
 
  /* put the argument types in reverse order in the tree 
     we use nreverse when finishing the type */
  TYPE_ARG_TYPES (method_type) = 
    tree_cons (NULL_TREE, actual_type, TYPE_ARG_TYPES (method_type));
}

/* see scil-callbacks.h */
tree
scil_complete_method_type (tree method_type, tree class_tree, bool varargs_p)
{
  tree arguments, result;

  /* last parameter if void indicates fixed argument list */
  if (!varargs_p)
    {
      TYPE_ARG_TYPES (method_type) = 
        tree_cons (NULL_TREE, void_type_node, TYPE_ARG_TYPES (method_type));
    }

  /* build function type with right argument order */
  arguments = nreverse (TYPE_ARG_TYPES (method_type));
  result = TREE_TYPE (method_type);
  return class_tree == NULL_TREE ? 
         build_function_type (result, arguments) :
         build_method_type_directly (class_tree, result, arguments);
}


void
scil_set_entry_point (tree method_decl)
{
  /* TODO: Create array out of main's arguments and pass this to
           method, if it has string[] as parameter type.
           Pass return value of method_decl (if any) on to main. 
           According to ECMA 335, entrypoints must have zero or one argument
           (of type string[] then) and return type void, int32, or uint32.
  */
  tree method_call = build_function_call_expr (method_decl, NULL_TREE);
  scil_append_statement_to_main (method_call);
}


/* see scil-callbacks.h */
bool
scil_start_method_def (tree method_decl, int max_stack)
{
  tree block;

  /* if this method is a runtime method */
  if (DECL_EXTERNAL (method_decl))
    return false;

  /* if this method has already been started to be defined */
  if (DECL_INITIAL (method_decl) != NULL_TREE)
    return false;

  DECL_INLINE (method_decl) = 1;
  TREE_ADDRESSABLE (method_decl) = 0;
  TREE_PUBLIC (method_decl) = 1;
  
  /* build the BLOCK for putting code and locals in */
  block = build_block (NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
  DECL_INITIAL (method_decl) = block;
  DECL_SAVED_TREE (method_decl) = 
    build3 (BIND_EXPR, void_type_node, NULL_TREE, NULL_TREE, NULL_TREE);

  current_function_decl = method_decl;
  current_param_type = TYPE_ARG_TYPES (TREE_TYPE (method_decl));
  scil_init_stacks (max_stack);
  return true;
}


/* see scil-callbacks.h */
tree
scil_add_method_parameter (const char *name)
{
  tree type, decl;

  type = TREE_VALUE (current_param_type);
  current_param_type = TREE_CHAIN (current_param_type);

  decl = scil_build_decl (PARM_DECL, get_identifier(name), type);
  DECL_ARG_TYPE (decl) = type; /* actual argument type */
  DECL_CONTEXT (decl) = current_function_decl;

  /* If the parameter's type is a memory alias struct we must mark the parameter */
  if (POINTER_TYPE_P (type) && TYPE_FIELDS (TREE_TYPE (type)) != NULL_TREE)
    {
      SCIL_MEMORY_ALIAS_P (decl) = 
        (SCIL_DECL_MEMORY_ALIAS (TYPE_FIELDS (TREE_TYPE (type))) != NULL_TREE);
    }

  /* put the arguments in reverse order in the tree 
     we use nreverse when finishing the method */
  TREE_CHAIN (decl) = DECL_ARGUMENTS (current_function_decl);
  DECL_ARGUMENTS (current_function_decl) = decl;

  return decl;
}

static tree
add_local (tree type, const char *header, int number)
{
  char buf[16];
  sprintf (buf, header, number);

  tree decl = scil_build_decl (VAR_DECL, get_identifier(buf), type);
  DECL_CONTEXT (decl) = current_function_decl;

  /* put the variables in reverse order in the tree 
     we use nreverse when finishing the method */
  tree block = DECL_INITIAL (current_function_decl);

  TREE_CHAIN (decl) = BLOCK_VARS (block);
  BLOCK_VARS (block) = decl;

  return decl;  
}


tree
scil_add_stack_variable (tree type)
{
  static int index = 0;
  return add_local (type, SCIL_ID_STACKVAR, index++);
}


/* see scil-callbacks.h */
tree
scil_add_method_variable (tree type)
{
  static int index = 0;
  return add_local (type, SCIL_ID_METHODVAR, index++);
}

/* see scil-callbacks.h */
void
scil_complete_method_def ()
{
  tree block, body;
  tree_stmt_iterator i;
 
  /* get arguments in right order */
  DECL_ARGUMENTS (current_function_decl) = 
    nreverse (DECL_ARGUMENTS (current_function_decl));

  /* get locals in right order */
  block = DECL_INITIAL (current_function_decl);
  BLOCK_VARS (block) = nreverse (BLOCK_VARS (block));
  
  body = DECL_SAVED_TREE (current_function_decl);
  BIND_EXPR_VARS (body) = BLOCK_VARS (block);

  /* Remove all labels that are not referred to, 
     nobody else seems to do this for us */
  body = BIND_EXPR_BODY (body);
  for (i = tsi_start (body); !tsi_end_p (i);)
    {
      tree stmt;
    
      stmt = tsi_stmt (i);
      if (TREE_CODE (stmt) == LABEL_EXPR 
          && !FORCED_LABEL (LABEL_EXPR_LABEL (stmt)))
        {
          tsi_delink (&i);
        }
      else
        {
          tsi_next (&i);
        }
    }

  /* do essential compilation */
  dump_function (TDI_original, current_function_decl);
  gimplify_function_tree (current_function_decl);
  dump_function (TDI_generic, current_function_decl);
  cgraph_finalize_function(current_function_decl, false);

  scil_free_stacks ();
}


void
scil_mark_class_constructor (tree method_def)
{
  /* TODO: care about static constructors that are not marked BeforeFieldInit */
  if (!DECL_STATIC_CONSTRUCTOR(method_def))
    {
      DECL_STATIC_CONSTRUCTOR (method_def) = 1;
      scil_append_statement_to_main (build_function_call_expr (method_def, NULL_TREE));
    }
}
