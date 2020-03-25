#include "build_trees.h"
#include "mono_structs.h"

tree
create_struct_mono_vtable (void)
{
  tree array_type;
        
  tree mono_vtable_type =  make_node (RECORD_TYPE);
  
  add_field (mono_vtable_type, ptr_mono_class_type_node, NULL_TREE, get_identifier ("klass"));

  /*
   * According to comments in gc_gcj.h, this should be the second word in
   * the vtable.
   */
  add_field (mono_vtable_type, ptr_type_node, NULL_TREE, get_identifier ("gc_descr"));

  /*
   * each object/vtable belongs to exactly one domain
   * TODO: change PTR_TYPE_NODE to ptr to struct MonoDomain
   */
  add_field (mono_vtable_type, ptr_type_node, NULL_TREE, get_identifier ("domain"));

 /* to store static class data */
  add_field (mono_vtable_type, ptr_type_node, NULL_TREE, get_identifier ("data"));

  /* System.Type type for klass */
  add_field (mono_vtable_type, ptr_type_node, NULL_TREE, get_identifier ("type"));
	
  add_field (mono_vtable_type,
	     uint16_type_node,
	     NULL_TREE,
	     get_identifier ("max_interface_id"));
	     
  add_field (mono_vtable_type,
	     uint8_type_node,
	     NULL_TREE,
	     get_identifier ("rank"));
 
  /*
   * remote: class is remotely activated
   * initialized: cctor has been run
   *
   * These fields are original bit fields, but we don't use them, so
   * we can put these fields in one field
   */
  add_field (mono_vtable_type,
	     char_type_node,
	     NULL_TREE,
	     get_identifier ("remote_initialized"));
	
  array_type = build_array_type (ptr_type_node,
				 build_index_type (integer_zero_node));
  add_field (mono_vtable_type, array_type, NULL_TREE, get_identifier ("vtable"));
	
  layout_type (mono_vtable_type);
	
  return mono_vtable_type;
}

tree
add_mono_class_fields (tree mono_class_type)
{      
  tree nat_int_array_type;
  
  /* construct type node for c-bitfield 
   * parameters: number of bits, unsigned */
  tree bitfield1_type_node = build_nonstandard_integer_type(1, 1);
  
  add_field (mono_class_type, bitfield1_type_node,
             NULL_TREE, get_identifier ("inited"));

  add_field (mono_class_type, ptr_mono_class_type_node,
             NULL_TREE, get_identifier ("parent"));
             
  add_field (mono_class_type, uint16_type_node,
             NULL_TREE, get_identifier ("interface_count"));
  
  add_field (mono_class_type, uint16_type_node, 
             NULL_TREE, get_identifier ("interface_id"));
  
  add_field (mono_class_type, uint16_type_node, 
             NULL_TREE, get_identifier ("max_interface_id"));
  
  nat_int_array_type = build_array_type (native_int_type_node,
                                 build_index_type (integer_zero_node));
  add_field (mono_class_type, build_pointer_type(nat_int_array_type),
             NULL_TREE, get_identifier ("interface_offsets"));

  add_field (mono_class_type, ptr_mono_class_type_node,
             NULL_TREE, get_identifier ("interfaces"));

  add_field (mono_class_type, native_int_type_node,
             NULL_TREE, get_identifier ("instance_size"));
                          
  add_field (mono_class_type, native_int_type_node,
             NULL_TREE, get_identifier ("vtable_size"));

  add_field (mono_class_type, uint32_type_node,
             NULL_TREE, get_identifier ("flags"));
  
  add_field (mono_class_type, mono_class_member_info_type_node,
             NULL_TREE, get_identifier ("field"));
  
  add_field (mono_class_type, mono_class_member_info_type_node,
             NULL_TREE, get_identifier ("method"));
  
  add_field (mono_class_type, mono_class_member_info_type_node,
             NULL_TREE, get_identifier ("property"));

  add_field (mono_class_type, mono_class_member_info_type_node,
             NULL_TREE, get_identifier ("event"));
  
  add_field (mono_class_type, build_pointer_type(mono_class_runtime_info_type_node),
             NULL_TREE, get_identifier ("runtime_info"));
 
  
  return mono_class_type;
}

tree 
create_struct_mono_class_member_info(void)
{
  tree member_info_type;
  
  member_info_type =  make_node (RECORD_TYPE);
  
  add_field (member_info_type, uint32_type_node,
             NULL_TREE, get_identifier ("first"));
  
  add_field (member_info_type, uint32_type_node,
             NULL_TREE, get_identifier ("count"));
  
  layout_type (member_info_type);
  
  return member_info_type;
}

tree
create_struct_mono_class_runtime_info(void)
{
  tree array_type;
  
  tree mono_class_runtime_info_type =  make_node (RECORD_TYPE);
  
  add_field (mono_class_runtime_info_type, uint16_type_node,
             NULL_TREE, get_identifier ("max_domain"));
  
  array_type = build_array_type (ptr_type_node,
         build_index_type (integer_zero_node));
  add_field (mono_class_runtime_info_type, array_type,
             NULL_TREE, get_identifier ("domain_vtables"));
  
  layout_type (mono_class_runtime_info_type);
  
  return mono_class_runtime_info_type;
}


/* Add required fields, to have a MonoObject */
void
add_mono_obj_fields (tree decl)
{
  add_field (decl,
	     ptr_mono_vtable_type_node,
	     NULL_TREE,
	     get_identifier ("__vtable"));

  add_field (decl,
	     ptr_type_node,
	     NULL_TREE,
	     get_identifier ("__synchronisation"));
}

void
add_mono_array_fields (tree record_type)
{
  add_field (record_type,
	     ptr_type_node,
	     NULL_TREE,
	     get_identifier ("__bounds"));

  add_field (record_type,
	     size_type_node,
	     NULL_TREE,
	     get_identifier ("__max_length"));
}

void
add_mono_string_fields (tree decl)
{
  add_field (decl,
	     size_type_node,
	     NULL_TREE,
	     get_identifier ("length"));
  add_field (decl,
	     build_array_type (int16_type_node,
			       build_index_type (integer_zero_node)),
	     NULL_TREE,
	     get_identifier ("chars"));
}
