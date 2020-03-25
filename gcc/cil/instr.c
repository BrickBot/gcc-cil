/* GCC headers */
#include "config.h"
#include "errors.h"

#include "build_trees.h"
#include "instr.h"
#include "parse_assembly.h"
#include "stack.h"
#include "mangle.h"


void
instr_call (ILToken token)
{
  ILMethod *method;
  tree method_name, return_type, params;

  method = ILMethod_FromToken (c_image, token);

  if (!method)
    {
      internal_error ("Error while processing method call");
    }

  method_name = mangle_function (method);
  return_type = parse_return_type (method);
  params = parse_parameters_types (method);

  make_method_call (method_name, return_type, params, false);
}

void
instr_ldftn (ILToken token)
{
  ILMethod *method;
  tree method_name, return_type, params;

  method = ILMethod_FromToken (c_image, token);

  if (!method)
    {
      internal_error ("Error while processing instruction 'ldftn'");
    }

  method_name = mangle_function (method);
  return_type = parse_return_type (method);
  params = parse_parameters_types (method);

  stack_push (create_function_ptr (method_name, params, return_type));
}

void
instr_callvirt (ILToken token)
{
  ILClass *owner;
  ILMethod *method;
  tree intf_mono_class_name, method_name, return_type, params;

  method = ILMethod_FromToken (c_image, token);

  if (!method)
    {
      internal_error ("Error while processing method call");
    }

  method_name = mangle_function (method);
  return_type = parse_return_type (method);
  params = parse_parameters_types (method);

  if (ILMethod_IsVirtual (method) && !ILMethod_IsRuntime (method))
  {
    owner = ILMemberGetOwner ((ILMember*)method);
    if (ILClass_IsInterface(owner))
    {
      intf_mono_class_name = mangle_mono_class(owner);      
      make_intf_method_call (return_type, params, intf_mono_class_name, method->index);
    }
    else
    {
      make_virtual_method_call (return_type, params, method->index);
    }
  }
  else
  {
    make_method_call (method_name, return_type, params, true);
  }
}

void
instr_ldvirtftn (ILToken token)
{
  ILClass *owner;
  ILMethod *method;
  tree intf_mono_class_name, method_name, return_type, params;

  method = ILMethod_FromToken (c_image, token);

  if (!method)
    {
      internal_error ("Error while processing instruction 'ldftn'");
    }

  method_name = mangle_function (method);
  return_type = parse_return_type (method);
  params = parse_parameters_types (method);
  
  owner = ILMemberGetOwner ((ILMember*)method);
  if (ILClass_IsInterface(owner))
  {
    intf_mono_class_name = mangle_mono_class(owner);
    stack_push (create_intf_method_ptr (stack_pop (NULL_TREE),
           return_type,
           params,
           intf_mono_class_name,
           method->index));
  }
  else
  {
    stack_push (create_virtual_method_ptr (stack_pop (NULL_TREE),
           return_type,
           params,
           method->index));
  }  
}

void
instr_ldstr (ILToken token)
{
  const char* str;
  unsigned long len;
  char pname[50];
  tree name;
  tree params;

  sprintf (pname, ".S%x", token & ~IL_META_TOKEN_MASK);
  name = get_identifier (pname);

  if (!IDENTIFIER_DECL (name)) {
    str = ILImageGetUserString (c_image, token & ~IL_META_TOKEN_MASK, &len);
    if (!str)
      internal_error ("Could not get string value");
    make_string (name, str, len);
  }

  params = build_tree_list (ptr_type_node, NULL_TREE);
  stack_push (build1 (ADDR_EXPR, 
		      build_pointer_type (TREE_TYPE (IDENTIFIER_DECL (name))),
		      IDENTIFIER_DECL (name)));
  make_method_call (get_identifier ("ldstr"),
		    ptr_type_node,
		    params,
		    false);
}	

void
instr_newarr (ILToken token)
{
  ILType *type =  ILClassToType (ILClass_FromToken (c_image, token));
  tree elem_type =  parse_type (type);
  make_new_array (elem_type);
}

void
instr_newobj (ILToken token)
{
  ILClass* klass;
  ILMethod *method;
  tree class_name, ctor_name, ctor_params;

  method = ILMethod_FromToken (c_image, token);

  if (!method)
    {
      internal_error ("Error while processing instruction newobj");
    }

  klass = ILMethod_Owner (method);
  class_name = get_namespace_class_name (klass);
  /* full name: includes namespace and class */
  ctor_name = mangle_function (method);
  ctor_params = parse_parameters_types (method);
	
  make_new_object (class_name, ctor_name, ctor_params);
}

void
instr_initobj (ILToken token)
{
  ILClass* klass;
  tree class_name, valueclass, params;

  klass = ILClass_FromToken (c_image, token);

  if (!klass)
    {
      internal_error ("Error while processing instruction newobj");
    }

  class_name = get_namespace_class_name (klass);

  /* Push the size onto the stack */
  valueclass = TYPE_VALUETYPE (IDENTIFIER_CLASS_DECL (class_name));
  stack_push (build_int_cst (integer_type_node,
			     TREE_INT_CST_LOW (TYPE_SIZE_UNIT (valueclass))));

  params = build_tree_list (size_type_node, NULL_TREE);
  params = tree_cons (build_pointer_type (void_type_node),
		      NULL_TREE, params);

  make_method_call (get_identifier ("initobj"),
		    void_type_node,
		    params,
		    false);
}

void
instr_ldfld (ILToken token)
{
  ILField *field;
  ILType *il_type;
  tree field_type, field_name, field_context;

  field = ILField_FromToken (c_image, token);

  if (!field)
    {
      internal_error ("Error while processing instruction ldfld");
    }

  il_type = ILFieldGetType (field);
  field_type = parse_type (il_type);
  field_context = get_class_decl (ILField_Owner (field));

  /* Create nodes to check null reference of this pointer */
  stack_push (create_null_reference_check (stack_pop (NULL_TREE)));

  field_name =  get_identifier (ILField_Name (field));
	
  stack_push (create_field_access (stack_pop(NULL_TREE), field_type, 
				   field_context, field_name));
}

void
instr_stfld (ILToken token)
{
  ILField *field;
  ILType *il_type;
  tree field_type, field_name, field_context;

  field = ILField_FromToken (c_image, token);

  if (!field)
    {
      internal_error ("Error while processing instruction stfld");
    }

  il_type = ILFieldGetType (field);
  field_type = parse_type (il_type);
  field_context = get_class_decl (ILField_Owner (field));

  field_name = get_identifier (ILField_Name (field));

  make_field_assignment (field_type, field_context, field_name);
}

void
instr_ldsfld (ILToken token)
{
  ILField *field;
  ILType *il_type;
  tree class_name, field_type, field_name;

  field = ILField_FromToken (c_image, token);

  if (!field)
    {
      internal_error ("Error while processing instruction ldsfld");
    }
	
  class_name = get_namespace_class_name (ILField_Owner (field));

  il_type = ILFieldGetType (field);
  field_type = parse_type (il_type);

  field_name = get_identifier (ILField_Name (field));

  stack_push (create_static_field_access (class_name,
					  field_type,
					  field_name));
}

void
instr_stsfld (ILToken token)
{
  ILField *field;
  ILType *il_type;
  tree class_name, field_type, field_name;

  field = ILField_FromToken (c_image, token);

  if (!field)
    {
      internal_error ("Error while processing instruction stsfld");
    }

  class_name = get_namespace_class_name (ILField_Owner (field));

  il_type = ILFieldGetType (field);
  field_type = parse_type (il_type);

  field_name = get_identifier (ILField_Name (field));

  make_static_field_assignment (class_name, field_type, field_name);
}
