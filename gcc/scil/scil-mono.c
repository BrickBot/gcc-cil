/* Function declarations for the runtime specific parts. */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "toplev.h"

#include "scil-config.h"
#include "scil-hooks.h"
#include "scil-rt.h"
#include "scil-callbacks.h"
#include "scil-builtins.h"
#include "scil-trees.h"
#include "scil-instructions.h"


void
scil_rt_make_builtins ()
{
  tree ftype, mscorlib, System, Console, Environment, Object, 
       CliHw, Hardware, Cpu, Stack;

  mscorlib = scil_register_assembly ("mscorlib");
  System = scil_register_namespace (mscorlib, "System");
  Object = scil_register_class (System, "Object");
  Console = scil_register_class (System, "Console");
  Environment = scil_register_class (System, "Environment");
  
  CliHw = scil_register_assembly ("CliHw");
  Hardware = scil_register_namespace (CliHw, "Hardware");
  Cpu = scil_register_class (Hardware, "Cpu");
  Stack = scil_register_class (Cpu, "Stack");


  /* static void System.Console.WriteLine (System.String) */
  ftype = scil_make_method_type (void_type_node);
  scil_add_method_parameter_type (ftype, TYPE_POINTER_TO (scil_string_type), /*byref = */ false);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Console, "WriteLine", ftype, 1, 1);
  
  /* static void System.Console.WriteLine (int32) */
  ftype = scil_make_method_type (void_type_node);
  scil_add_method_parameter_type (ftype, scil_int32_type, /*byref = */ false);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Console, "WriteLine", ftype, 1, 1);

  /* static void System.Console.WriteLine (bool) */
  ftype = scil_make_method_type (void_type_node);
  scil_add_method_parameter_type (ftype, scil_bool_type, /*byref = */ false);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Console, "WriteLine", ftype, 1, 1);

  /* static void System.Environment.Exit (int32) */
  ftype = scil_make_method_type (void_type_node);
  scil_add_method_parameter_type (ftype, scil_int32_type, /*byref = */ false);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Environment, "Exit", ftype, 1, 1);

  /* void System.Object.Finalize () */
  ftype = scil_make_method_type (void_type_node);
  ftype = scil_complete_method_type (ftype, Object, /*varargs = */ false);
  scil_register_method_impl (Object, "Finalize", ftype, 1, 1);

  /* System.String System.Object.ToString () */
  ftype = scil_make_method_type (TYPE_POINTER_TO (scil_string_type));
  ftype = scil_complete_method_type (ftype, Object, /*varargs = */ false);
  scil_register_method_impl (Object, "ToString", ftype, 1, 1);

  /* int32 System.Object.GetHashCode () */
  ftype = scil_make_method_type (scil_int32_type);
  ftype = scil_complete_method_type (ftype, Object, /*varargs = */ false);
  scil_register_method_impl (Object, "GetHashCode", ftype, 1, 1);

  /* void *<SCIL_ID_NEWOBJ> (void*, size_t)*/
  ftype = scil_make_method_type (ptr_type_node);
  scil_add_method_parameter_type (ftype, ptr_type_node, /*byref = */ false);
  scil_add_method_parameter_type (ftype, size_type_node, false);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (NULL_TREE, SCIL_ID_NEWOBJ, ftype, 1, 0);
  
  /* static void Hardware.Cpu.Sleep () */
  ftype = scil_make_method_type (void_type_node);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Cpu, "Sleep", ftype, 1, 1);

  /* static Stack Hardware.Cpu.InitStack (ProgramCounterDelegate, ushort) */
  ftype = scil_make_method_type (Stack);
  /* ProgramCounterDelegate is attributed StaticDelegate, 
     hence its actual type is ptr_type_node */
  scil_add_method_parameter_type (ftype, ptr_type_node, /*byref = */ false);
  scil_add_method_parameter_type (ftype, scil_uint16_type, /*byref = */ false);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Cpu, "InitStack", ftype, 1, 1);

  /* static void Hardware.Cpu.InvokeScheduler () */
  ftype = scil_make_method_type (void_type_node);
  ftype = scil_complete_method_type (ftype, NULL_TREE, /*varargs = */ false);
  scil_register_method_impl (Cpu, "InvokeScheduler", ftype, 1, 1);
}


tree 
scil_rt_make_vtable (tree type, unsigned int size, tree base_vtable)
{
  tree id, vtable;

  id = scil_get_nested_identifier (TYPE_NAME (type), SCIL_ID_VTABLE);

  vtable = base_vtable == NULL_TREE ?
           scil_make_array_constant (id, ptr_type_node, size) :
           scil_make_array_constant_with_init (id, ptr_type_node, 
             size + scil_get_array_constant_length (base_vtable), base_vtable);

  return vtable;
}

void 
scil_rt_set_vtable_slot (tree vtable, unsigned int index, tree address)
{
  scil_set_array_constant_element (vtable, index, address);
}

unsigned int 
scil_rt_new_vtable_slot (tree vtable)
{
  return (unsigned int) scil_add_array_constant_element (vtable);
}

tree 
scil_rt_get_vtable_content (tree object, unsigned int index)
{
  tree index_expr, vtable, field, atype, ref;

  field = TYPE_FIELDS (scil_object_type);
  index_expr = build_int_cst (size_type_node, index);
  atype = build_array_type (ptr_type_node, build_index_type (size_zero_node));

  /* get vtable */
  vtable = build1 (NOP_EXPR, TYPE_POINTER_TO (scil_object_type), object);
  vtable = build1 (INDIRECT_REF, scil_object_type, vtable);
  vtable = build3 (COMPONENT_REF, ptr_type_node, vtable, field, NULL_TREE);
  vtable = build1 (NOP_EXPR, build_pointer_type (atype), vtable);
  vtable = build1 (INDIRECT_REF, atype, vtable);

  /* build virtual reference */
  ref = build4 (ARRAY_REF, ptr_type_node, vtable, index_expr, NULL_TREE, NULL_TREE);
  return build3 (OBJ_TYPE_REF, ptr_type_node, ref, object, index_expr);
}


void
scil_rt_init_object_type ()
{
  scil_add_field (scil_object_type, "vtable", ptr_type_node);
  scil_add_field (scil_object_type, "synchronization", ptr_type_node);
}

void 
scil_rt_init_constant_object (tree constructor, tree rt_fields, tree type ATTRIBUTE_UNUSED)
{
  scil_init_next_field (constructor, rt_fields, integer_zero_node);
  rt_fields = TREE_CHAIN (rt_fields); 
  scil_init_next_field (constructor, rt_fields, integer_zero_node);
}


void
scil_rt_init_string_type ()
{
  scil_add_field (scil_string_type, "length", size_type_node);
  scil_add_field (scil_string_type, "chars", scil_string_literal_type);
}


void 
scil_rt_init_constant_string (tree constructor, tree rt_fields, tree literal, 
                              unsigned int length)
{
  tree len = build_int_cst (size_type_node, length);
  scil_init_next_field (constructor, rt_fields, len);
  rt_fields = TREE_CHAIN (rt_fields); 
  scil_init_next_field (constructor, rt_fields, literal);
}

tree
scil_rt_new_object (tree type, tree vtable)
{
  tree alloc, object, parameters, ptype, expr, field;

  /* take address of vtable as void* */
  vtable = build1 (ADDR_EXPR, build_pointer_type (TREE_TYPE (vtable)), vtable);
  vtable = build1 (NOP_EXPR, ptr_type_node, vtable);

  /* build call for object allocation */
  ptype = build_pointer_type (type);
  alloc = TREE_VALUE (SCIL_DECL (get_identifier (SCIL_ID_NEWOBJ)));
  parameters = tree_cons (NULL_TREE, TYPE_SIZE_UNIT (type), NULL_TREE);
  parameters = tree_cons (NULL_TREE, vtable, parameters);
  object = build_function_call_expr (alloc, parameters);
  object = build1 (NOP_EXPR, ptype, object);
  object = build1 (SAVE_EXPR, ptype, object);

  /* set vtable */
  field = TYPE_FIELDS (scil_object_type);
  expr = build1 (NOP_EXPR, TYPE_POINTER_TO (scil_object_type), object);
  expr = build1 (INDIRECT_REF, scil_object_type, expr);
  expr = build3 (COMPONENT_REF, ptr_type_node, expr, field, NULL_TREE);
  expr = build2 (MODIFY_EXPR, ptr_type_node, expr, vtable);

  scil_append_stmt (expr);

  return object;
}
