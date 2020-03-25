/*
  CIL Compiler

  Process productions and tokens of the given grammar.

  Copyright (C) 2003 Jan Möller
  Copyright (C) 2005, 2006 Martin v. Löwis

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.

  In other words, you are welcome to use, share and improve this program.
  You are forbidden to forbid anyone else to use, share and improve
  what you give them.   Help stamp out software-hoarding!

  ---------------------------------------------------------------------------

  Written by Jan Moeller 2003, based in part on other
  parts of the GCC compiler.
*/

#include <stdio.h>

/* cil-decl.h includes: config.h system.h tree.h */
#include "cil-decl.h"

/* GCC headers.  */
#include "toplev.h"
#include "rtl.h"
#include "errors.h"
#include "target.h"
#include "ggc.h"
#include "tree-gimple.h"
#include "tree-pass.h"
#include "tree-dump.h"
#include "cgraph.h"

/* Include expr.h after insn-config.h so we get HAVE_conditional_move.  */
#include "expr.h"

/* CIL headers */
#include "build_trees.h"
#include "mono_structs.h"
#include "stack.h"
#include "mangle.h"

/* pnet headers */
#include "include/il_program.h"

/* typedefs */
typedef struct {
	char *name;
	tree init_val;
} Field;

/* common constants for creation of mono class */
#define MEMBER_INFO_METHOD_FIRST 0


/* Declaration of local functions */
static tree add_interface_slot (tree interfaces_list, int index, tree intf_mono_class_decl);
static tree make_record (void);
static tree create_array_bounds_check (tree array_ref, tree index);
static tree create_field_access_ind (tree obj_ref, tree type, int index);
static tree create_function_decl (tree function_name, tree param_type_list, tree return_type);
static tree create_function_type (tree return_type, tree param_type_list);
static tree create_function_call (tree function_ptr, bool this_check);
static tree create_mono_class_interfaces_entries (tree class_name);
static void finish_function_call (tree function_call);
static tree get_array_length (tree array_ref);
static tree get_class_data_decl (tree class_name);
static ILClass* get_ILClass_from_class_name (tree class_name);
static tree get_mono_class (tree class_name);
static tree make_mono_class_interfaces (tree intf_name_list);
static tree make_mono_class_member_info (int first, int count);
static tree make_mono_class_parent_ref (tree parent_name);
static tree make_record_init_val(tree record_type_node, Field fields[], int size);
static tree make_static_fields (tree s_fields_decl);
static tree make_param_list (tree function_decl, tree params);
static tree lookup_field_decl (tree decl, tree context, tree name);
static tree lookup_field_decl_ind (tree decl, int index);
static tree lookup_label (tree label_name);

/* Chain of local variables */
static GTY(()) tree current_function_locals;
static GTY(()) tree current_function_body;

tree
make_complex_identifier (tree ident_1, ...)
{
  va_list args;
  int num_args = 1;
  size_t length = IDENTIFIER_LENGTH (ident_1);
  tree ident;
  char *new_ident_str; /*, *tmp_ident_str; */

  va_start (args, ident_1);

  while ((ident = va_arg (args, tree)))
    {
      ++num_args;
      length += IDENTIFIER_LENGTH (ident);
    }
  va_end (args);
	
  if (num_args < 2)
    {
      internal_error ("make_complex_identifier: needed at least 2 identifiers");
    }

  new_ident_str = alloca (length + num_args );
			    
  strcpy (new_ident_str, IDENTIFIER_POINTER (ident_1));

  va_start (args, ident_1);
  while ((ident = va_arg (args, tree)))
    {
      strcat (new_ident_str, "$");
      strcat (new_ident_str, IDENTIFIER_POINTER (ident));
    }
  va_end (args);

  return get_identifier (new_ident_str);
}

static tree
make_record (void)
{
  tree r = make_node (RECORD_TYPE);
  TYPE_LANG_SPECIFIC (r) = GGC_CNEW(struct lang_type);
  return r;
}

struct lang_type*
cil_type_lang_specific (tree t)
{
  struct lang_type* result = TYPE_LANG_SPECIFIC (t);
  if (result == NULL) 
    TYPE_LANG_SPECIFIC (t) = result = GGC_CNEW(struct lang_type);
  return result;
}
      

/* VARS is a tree_list: PURPOSE is a '*_TYPE_NODE'
   VALUE is an 'IDENTIFIER_NODE' */
void
add_local_vars (tree vars)
{
  int i = 0;

  for (; vars; vars = TREE_CHAIN (vars)) 
    {
      tree var_decl;
      /* Store VAR_DECL in LOCAL_VARS to access variable by number */
      local_vars[i] = var_decl = build_decl (VAR_DECL,
					     TREE_VALUE (vars),
					     TREE_PURPOSE (vars));
      /* Set 'FUNCTION_DECL' in which the variable is declared */
      DECL_CONTEXT (var_decl) = current_function_decl;

      /* Store VAR_DECL in symbol table entry to access variable later by name */
      IDENTIFIER_DECL (TREE_VALUE (vars)) = var_decl;

      /* Add local variable to list of variables for the function. */
      current_function_locals = chainon (current_function_locals,
					 var_decl);
    
      i++;
    }

  num_of_local_vars = i ;
}

tree
create_value_decl (void)
{
  return make_record ();
}

tree
create_class_decl (ILClass* klass, tree class_name)
{
  tree class_decl = IDENTIFIER_CLASS_DECL (class_name);

  if (!class_decl)
    {
      class_decl =  make_record ();

      IDENTIFIER_CLASS_DECL (class_name) = class_decl;
      TYPE_ILClass (class_decl) = klass;

      add_mono_obj_fields (
			   IDENTIFIER_CLASS_DECL (class_name));
			
    }

  return class_decl;
}

tree
create_static_fields_decl (ILClass *klass, tree class_name)
{
  tree s_fields_decl =  make_record ();
  TYPE_ILClass (s_fields_decl) = klass;

  IDENTIFIER_STATIC_FIELDS_DECL (class_name) = s_fields_decl;
        
  return s_fields_decl;
}

void
finish_class (tree class_name)
{
  tree class_decl, s_fields_decl;
	
  s_fields_decl = IDENTIFIER_STATIC_FIELDS_DECL (class_name);

  if (s_fields_decl)
    {
      layout_type (s_fields_decl);
    }

  class_decl = IDENTIFIER_CLASS_DECL (class_name);

  layout_type (class_decl);
}

tree
add_field (tree record, tree type, tree context, tree name)
{
  tree rec_fields = TYPE_FIELDS (record);

  tree field = build_decl (FIELD_DECL, name, type);
  DECL_CONTEXT (field) = record;
  DECL_FCONTEXT (field) = context;

  rec_fields = chainon (rec_fields, field);

  TYPE_FIELDS (record) = rec_fields;

  return field;
}

/* type of DECL must be 'RECORD_TYPE' */
tree
lookup_field_decl (tree decl, tree context, tree name)
{
  tree field = TYPE_FIELDS (decl);
	
  while (field && (name != DECL_NAME (field) || context != DECL_FCONTEXT (field)))
    {
      field = TREE_CHAIN (field);
    }
	
  if (!field)
    {
      internal_error ("field '%s' not found",
		      IDENTIFIER_POINTER (name));
    }
	
  return field;
}

/* type of DECL must be 'RECORD_TYPE' */
tree
lookup_field_decl_ind (tree decl, int index)
{
  tree field = TYPE_FIELDS (decl);

  int i;
  for (i = 1; i < index; ++i)
    {
      if (TREE_CHAIN (field))
	{
	  field = TREE_CHAIN (field);
	}
      else
	{
	  internal_error ("field index out of range");
	}
    }
	
  return field;
}

/*
 * gets global variable that stores static class data
 *
 * CLASS_NAME: identifier node
 */

tree
get_class_data_decl (tree class_name)
{
  /* TODO: consider multiple appdomains */

  tree decl = IDENTIFIER_STATIC_FIELDS_DECL (class_name);
  
  ILClass* klass = get_ILClass_from_class_name (class_name);
  /* referenced by field 'data' of struct MonoVTable */ 
  tree class_data_name = mangle_class_data (klass);
  
  tree class_data_decl = IDENTIFIER_DECL (class_data_name);
  
  if (class_data_decl == NULL_TREE)
  {
    class_data_decl = build_decl (VAR_DECL,
         class_data_name,
         decl);
    IDENTIFIER_DECL (class_data_name) = class_data_decl;        
  }
  
  return class_data_decl;
}

/*
 * create static fields in a record as global variable
 *
 * CLASS_NAME: - identifier node
 */

tree
make_static_fields (tree class_name)
{
  tree field;

  tree init_vals = NULL_TREE;
        
  tree fields_decl = IDENTIFIER_STATIC_FIELDS_DECL (class_name);
  
  tree class_data_decl = get_class_data_decl (class_name);

  /* initial value of a constant is stored in DECL_INITIAL */

  for (field = TYPE_FIELDS (fields_decl); field; field = TREE_CHAIN (field))
  {
    tree decl_init = DECL_INITIAL (field);

    if (decl_init)
    {
      init_vals = chainon (init_vals,
			       build_tree_list (field, decl_init));

      DECL_INITIAL (field) = NULL_TREE;
    }
  }

  /* TODO replace with build_constructor */
  init_vals = build_constructor_from_list (fields_decl, init_vals);

  TREE_PUBLIC (class_data_decl) = 1;
  TREE_STATIC (class_data_decl) = 1;
  DECL_INITIAL (class_data_decl) = init_vals;

  rest_of_decl_compilation (class_data_decl, 0, 0);
	
  return build1 (ADDR_EXPR, build_pointer_type (fields_decl), class_data_decl);
}

static tree
make_mono_class_member_info (int first, int count)
{
	/* number of class fields to initialize */
	#define MEMBER_INFO_FIELD_COUNT  2
	Field fields[MEMBER_INFO_FIELD_COUNT];
	
	
	fields[0].name = (char*) "first";
	fields[0].init_val = build_int_cst(integer_type_node, first);
	
	fields[1].name = (char*) "count";
	fields[1].init_val = build_int_cst(integer_type_node, count);


	return make_record_init_val (	mono_class_member_info_type_node, 
															fields, MEMBER_INFO_FIELD_COUNT);
}

void
make_mono_class_runtime_info (tree mono_class_runtime_info_name, tree mono_vtable_name)
{
	/* number of class fields to initialize */
	#define RUNTIME_INFO_FIELD_COUNT  2
	#define DEFAULT_DOMAIN_ID 0
	Field fields[RUNTIME_INFO_FIELD_COUNT];
	
	int domain_index;
  tree runtime_info;
	tree mono_vtable, domain_vtables_entry, domain_vtables_entries;
	tree ptr_mono_vtable_array_type, runtime_info_init_val;
	
	
	mono_vtable = IDENTIFIER_DECL (mono_vtable_name);
	
	fields[0].name = (char*) "max_domain";
	fields[0].init_val = build_int_cst(integer_type_node, DEFAULT_DOMAIN_ID);
	
	fields[1].name = (char*) "domain_vtables";
	
	
	domain_vtables_entry = build1 (	ADDR_EXPR, 
										build_pointer_type(TREE_TYPE(mono_vtable)), 
										mono_vtable);

	domain_index = 0;
	domain_vtables_entries = build_tree_list (
                     build_int_cst(integer_type_node, domain_index), 
                     domain_vtables_entry);
	
	ptr_mono_vtable_array_type = build_array_type (ptr_mono_vtable_type_node,
                                 	build_index_type (integer_zero_node));
                                 	
	fields[1].init_val = build_constructor_from_list ( 
										ptr_mono_vtable_array_type, 
										domain_vtables_entries);


	runtime_info_init_val = make_record_init_val (	
                                mono_class_runtime_info_type_node, 
															fields, RUNTIME_INFO_FIELD_COUNT);
                              
/*  return build1 ( ADDR_EXPR, 
                  build_pointer_type (mono_class_runtime_info_type_node),
                  runtime_info_init_val);*/

  runtime_info = build_decl (VAR_DECL, mono_class_runtime_info_name, 
                             mono_class_runtime_info_type_node);
  
  IDENTIFIER_DECL (mono_class_runtime_info_name) = runtime_info;
  
  TREE_STATIC (runtime_info) = 1;                
  
  DECL_INITIAL (runtime_info) = runtime_info_init_val;
  
  rest_of_decl_compilation (runtime_info, 0, 0);
}

static tree
make_mono_class_parent_ref (tree parent_name)
{
  tree result;
  
  if (parent_name) 
  {
    result = build1 (ADDR_EXPR, ptr_mono_class_type_node,
                     IDENTIFIER_DECL (parent_name));
  }
  else 
  {
    result = null_pointer_node;
  }
  
  return result;
}

static tree
add_interface_slot (tree interfaces_list, int index, tree intf_mono_class_decl)
{
  tree slot, init_val;

  init_val = build1 ( ADDR_EXPR, 
                    build_pointer_type(TREE_TYPE(intf_mono_class_decl)), 
                    intf_mono_class_decl);
                           
  slot = build_tree_list (build_int_cst(integer_type_node, index), init_val);

  return chainon (interfaces_list, slot);
}

static tree
create_mono_class_interfaces_entries (tree intf_mono_class_name_list)
{
  int index;
  tree intf_mono_class_name, intf_mono_class_decl, result = NULL_TREE;

  index = 0;
  while (intf_mono_class_name_list) {
    
    intf_mono_class_name = TREE_VALUE (intf_mono_class_name_list);
      
    intf_mono_class_decl = IDENTIFIER_DECL (intf_mono_class_name);
      
      result = add_interface_slot (result, index, intf_mono_class_decl);
      
      /* iterate, define next interface */
      index++;
      intf_mono_class_name_list = TREE_CHAIN (intf_mono_class_name_list);
  }
  
  return result;
}

/* makes the init val for the interfaces field of a MonoClass */
static tree
make_mono_class_interfaces (tree intf_name_list)
{
  tree result, array_type;
  tree interfaces_entries, interfaces_entries_constr;
  
  if (! intf_name_list)
  {
    result = null_pointer_node;
  }
  else
  {
    interfaces_entries = create_mono_class_interfaces_entries (intf_name_list); 
       
    array_type = build_array_type (ptr_mono_class_type_node,
                                   build_index_type (integer_zero_node));
    
    interfaces_entries_constr = build_constructor_from_list (
                                   array_type, interfaces_entries);
    
    result = build1 ( ADDR_EXPR, ptr_mono_class_type_node,
                      interfaces_entries_constr);
  }
  
  return result;
}

/*
 * create MonoClass as global variable
 * 
 * MONO_CLASS_NAME: - identifier node
 *                  - mangeled name of mono_class
 */ 
tree 
create_mono_class_decl (tree mono_class_name)
{
	tree mono_class;
  
  mono_class = build_decl (VAR_DECL, mono_class_name, mono_class_type_node);
  
  /* this flag seems to be important 
   * in make_mono_vtable a reference to mono_class is created 
   * but at this time mono_class is not made/initialized. 
   * This flag ensures the possibility to create a reference anyway */
  TREE_STATIC (mono_class) = 1;
  
  IDENTIFIER_DECL (mono_class_name) = mono_class;
  
  return mono_class;
}

/*
 * make/initialize MonoClass for a class / non-interface
 * 
 * mono_class_name: - identifier node
 *                  - mangeled name of a class
 * 
 * intf_name_list:  - a list of tree list nodes representing 
 *                    all interfaces implemented by the class
 *                  - TREE_VALUE (list_node) -> identifier node, mangled name of interface
 * 
 * parent_name:  - identifier node
 *               - mangeled name of parent/base class
 * 
 * method_count: - number of methods defined in the class
 * 
 * mono_vtable_name: - identifier node
 *                   - mangeled name of the corresponding vtable
 */ 
void 
make_mono_class (tree mono_class_name, tree intf_name_list, tree parent_name,
								int method_count, int instance_size, int vtable_size,
                 tree mono_class_runtime_info_name )
{
	/* number of class fields to initialize */
	#define FIELD_COUNT  8

	int interface_count;
	tree mono_class, mono_class_init_val, runtime_info;
	
	Field fields[FIELD_COUNT];
	
	
  fields[0].name = (char*) "parent";
  fields[0].init_val = make_mono_class_parent_ref (parent_name);  
  
  interface_count = list_length (intf_name_list);                     	
	fields[1].name = (char*) "interface_count";
	fields[1].init_val = build_int_cst(short_integer_type_node, interface_count);
	
  fields[2].name = (char*) "interface_id";
  fields[2].init_val = build_int_cst(short_integer_type_node, 1000);
  
	fields[3].name = (char*) "interfaces";
	fields[3].init_val = make_mono_class_interfaces (intf_name_list);

  fields[4].name = (char*) "instance_size";
  fields[4].init_val = build_int_cst(integer_type_node, instance_size);
    	
  fields[5].name = (char*) "vtable_size";
  fields[5].init_val = build_int_cst(integer_type_node, vtable_size);
  
  fields[6].name = (char*) "method";
	fields[6].init_val = make_mono_class_member_info (
															MEMBER_INFO_METHOD_FIRST, method_count);
  
  runtime_info = IDENTIFIER_DECL (mono_class_runtime_info_name);
  	fields[7].name = (char*) "runtime_info";
	fields[7].init_val = build1 ( ADDR_EXPR, 
                    build_pointer_type(TREE_TYPE(runtime_info)), 
                    runtime_info);
  																																										
	mono_class_init_val = make_record_init_val (mono_class_type_node, 
																						fields, FIELD_COUNT);
  
  mono_class = IDENTIFIER_DECL (mono_class_name); 
   
  TREE_PUBLIC (mono_class) = 1;
  TREE_STATIC (mono_class) = 1;
  
  DECL_INITIAL (mono_class) = mono_class_init_val;
  
  rest_of_decl_compilation (mono_class, 0, 0);
} 

/*
 * make/initialize MonoClass for an interface
 * 
 * mono_class_name: - identifier node
 *                  - mangeled name of a class
 * 
 * intf_name_list:  - a list of tree list nodes representing 
 *                    all base interfaces of this interface
 *                  - TREE_VALUE (list_node) -> identifier node, mangled name of interface
 *
 * method_count: - number of methods defined in the class
 */
void 
make_intf_mono_class (tree mono_class_name, tree intf_name_list, int method_count)
{
  #define INTF_MONO_CLASS_FIELD_COUNT  3
  Field fields[INTF_MONO_CLASS_FIELD_COUNT];
  
  tree mono_class, mono_class_init_val;

  
  fields[0].name = (char*) "interfaces";
  fields[0].init_val = make_mono_class_interfaces (intf_name_list);

  fields[1].name = (char*) "flags";
  fields[1].init_val = build_int_cst(integer_type_node, MONO_TYPE_ATTRIBUTE_INTERFACE);
  
  fields[2].name = (char*) "method";
  fields[2].init_val = make_mono_class_member_info (
                              MEMBER_INFO_METHOD_FIRST, method_count);
                              
  mono_class_init_val = make_record_init_val (mono_class_type_node, 
                                            fields, INTF_MONO_CLASS_FIELD_COUNT);
                                              
  
  mono_class = create_mono_class_decl (mono_class_name);
    
  TREE_PUBLIC (mono_class) = 1;
  TREE_STATIC (mono_class) = 1;
  
  DECL_INITIAL (mono_class) = mono_class_init_val;
  
  rest_of_decl_compilation (mono_class, 0, 0);
}

void
make_ext_mono_class (tree mono_class_name)
{
	tree mono_class;
  
  mono_class = build_decl (VAR_DECL, mono_class_name, mono_class_type_node);

  IDENTIFIER_DECL (mono_class_name) = mono_class;

	DECL_EXTERNAL (mono_class) = 1;

	rest_of_decl_compilation (mono_class, 0, 0);
}

static tree
make_record_init_val (tree record_type_node, Field fields[], int size)
{
	int index;
	tree init_vals, field_decl, inited_field;
	
	init_vals = NULL_TREE;
	
	for (index = 0; index < size; index++) {
		
		field_decl = lookup_field_decl (record_type_node,
						NULL_TREE, get_identifier(fields[index].name));
		
		inited_field = build_tree_list (field_decl, fields[index].init_val);
		
		init_vals = chainon (init_vals, inited_field);
	}
	
	return build_constructor_from_list  (mono_class_type_node, init_vals);
}

static tree
get_mono_class (tree class_name)
{
  ILClass *klass = get_ILClass_from_class_name (class_name);
  
  return get_mono_class_from_ILClass (klass);
}

static ILClass*
get_ILClass_from_class_name (tree class_name)
{
	tree class_decl = IDENTIFIER_CLASS_DECL (class_name);
	
	return TYPE_ILClass (class_decl);
}

tree
get_mono_class_from_ILClass (ILClass *klass)
{
  tree mono_class_name = mangle_mono_class (klass);

  return IDENTIFIER_DECL (mono_class_name);
}

/*
 * create MonoVTable as global variable
 *
 * CLASS_NAME: - identifier node
 *             - name: [assembly name]$[namespace name]$[class name]
 * 
 * VT_NAME: - identifier node
 *          - mangeled name of vtable
 *
 * VTABLE_ENTRIES: - is a tree list
 *                 - purpose: index
 *                 - value: function pointer
 */
void
make_mono_vtable (tree class_name, tree vt_name, tree vtable_entries)
{
  tree s_fields_decl;
  tree klass_field_decl, data_field_decl, vtable_field_decl;
  tree klass_init_val = NULL_TREE;
  tree data_init_val = NULL_TREE;
  tree vtable_init_vals = NULL_TREE;
  tree mono_vtable_init_vals = NULL_TREE;

  tree mono_vtable = build_decl (VAR_DECL,
				 vt_name,
				 mono_vtable_type_node);
  
  klass_field_decl = lookup_field_decl (mono_vtable_type_node,
            NULL_TREE,
            get_identifier("klass"));
  
  klass_init_val = build1 (ADDR_EXPR,
                           TREE_TYPE (klass_field_decl),
                           get_mono_class(class_name));

  mono_vtable_init_vals = chainon (mono_vtable_init_vals,
           build_tree_list (klass_field_decl, klass_init_val));

  s_fields_decl = IDENTIFIER_STATIC_FIELDS_DECL (class_name);

  if (s_fields_decl)
  {
      /* intitial value for field data -> static fields */
      data_init_val = make_static_fields (class_name);
            
      data_field_decl = lookup_field_decl (mono_vtable_type_node,
            NULL_TREE,
            get_identifier("data"));

      mono_vtable_init_vals = chainon (mono_vtable_init_vals,
              build_tree_list (data_field_decl, data_init_val));
  }
				
  vtable_field_decl = lookup_field_decl (mono_vtable_type_node,
					 NULL_TREE,
					 get_identifier("vtable"));


  /* intitial values for vtable array */
  /* TODO replace with build_constructor */
  vtable_init_vals = build_constructor_from_list
    (TREE_TYPE (vtable_field_decl), vtable_entries);
  
  mono_vtable_init_vals = chainon (mono_vtable_init_vals,
				   build_tree_list (vtable_field_decl,
						    vtable_init_vals));

  /* TODO repace with build_constructor */
  mono_vtable_init_vals = build_constructor_from_list 
    (mono_vtable_type_node, mono_vtable_init_vals);


  TREE_PUBLIC (mono_vtable) = 1;
  TREE_STATIC (mono_vtable) = 1;
  DECL_INITIAL (mono_vtable) = mono_vtable_init_vals;
  
  IDENTIFIER_DECL (vt_name) = mono_vtable;

  rest_of_decl_compilation (mono_vtable, 0, 0);
}

/* CTOR_PARAMS is a tree_list: PURPOSE is a '*_TYPE_NODE' */
void
make_new_object (tree class_name, tree ctor_name, tree ctor_params)
{
  tree obj_ref_type, params;

  /* pop constructor arguments */
  tree ctor_args = NULL_TREE;
  int num_of_params = list_length (ctor_params) - 1;
	
  /* save arguments as tree list*/
  if (num_of_params)
    {
      int i;
      for (i = 0; i < num_of_params; ++i)
	{
	  ctor_args = tree_cons (NULL_TREE,
				 stack_pop (NULL_TREE),
				 ctor_args);
	}
    }

  obj_ref_type = build_pointer_type (
				     IDENTIFIER_CLASS_DECL (class_name));

  params = build_tree_list (ptr_mono_class_type_node, NULL_TREE);

  stack_push (build1 (ADDR_EXPR,
		      ptr_mono_class_type_node,
		      get_mono_class (class_name)));

  make_method_call (get_identifier ("newobj"),
		    obj_ref_type,
		    params,
		    false);

  make_instr_dup ();
	
  /* push contructor arguments */
  if (ctor_args)
    {
      for (; ctor_args; ctor_args = TREE_CHAIN (ctor_args))
	{
	  stack_push (TREE_VALUE (ctor_args));
	}
    }

  make_method_call (ctor_name, void_type_node, ctor_params, true);
}

tree
create_array_decl (tree elem_type)
{
  tree field_name, field_decl;
  tree array_decl =  make_record ();
  TYPE_ARRAYTYPE (elem_type) = array_decl;

  add_mono_obj_fields (array_decl);
  add_mono_array_fields (array_decl);

  field_name = get_identifier ("__vector");
					
  field_decl = add_field (array_decl, elem_type, NULL_TREE, field_name);
  DECL_ALIGN (field_decl) = TYPE_ALIGN (double_type_node);
  DECL_USER_ALIGN (field_decl) = 1;
	
  layout_type (array_decl);

  return array_decl;
}

/* ARRAY_TYPE is a 'TYPE_NODE' */
void
make_new_array (tree elem_type)
{
  tree params = build_tree_list (size_type_node, NULL_TREE);
  params = tree_cons (size_type_node, NULL_TREE, params);

  stack_push (build_int_cst (integer_type_node, 
			     TREE_INT_CST_LOW (TYPE_SIZE_UNIT (elem_type))));

  make_method_call (get_identifier ("newarr"),
		    ptr_type_node,
		    params,
		    false);
}

/* Create a variable identifying a string literal.
   NAME is the variable name, STR the pointer to the UTF-16BE encoded
   string, and LEN is the number of bytes in the string.
*/
void
make_string (tree name, const char* str, unsigned long len)
{
  tree vals, field, chars;
  unsigned short* ustr = (unsigned short*)str;
  unsigned int i;

  tree decl = build_decl (VAR_DECL, name, mono_string_type_node);
  IDENTIFIER_DECL (name) = decl;
  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;

  /* __vtable */
  field = TYPE_FIELDS (mono_string_type_node);
  vals = build_tree_list (field, null_pointer_node);
  /* __synchronisation */
  field = TREE_CHAIN (field);
  chainon (vals, build_tree_list (field, null_pointer_node));
  /* length */
  field = TREE_CHAIN (field);
  chainon (vals, build_tree_list (field, 
				  build_int_cst (integer_type_node,
						 len)));
  /* values */
  field = TREE_CHAIN (field);
  chars = NULL_TREE;
  for (i = 0; i < len; i++) {
    tree value = build_int_cst (int16_type_node,
				IL_READ_INT32 (ustr+i));
    chars = chainon (chars, 
		     build_tree_list (build_int_cst (integer_type_node, i),
				      value));
  }
  /* TODO replace with build_constructor */
  chars = build_constructor_from_list (TREE_TYPE (field), chars);
  chainon (vals, build_tree_list (field, chars));

  DECL_INITIAL (decl) = build_constructor_from_list (mono_string_type_node, vals);
  rest_of_decl_compilation (decl, 1, 0);
}

/* PARAMS is a tree_list: PURPOSE is a '*_TYPE_NODE' */
tree
create_function_type (tree params, tree return_type)
{
  tree param_type_list = NULL_TREE;

  /* Create parameter's type list */
  for (; params; params = TREE_CHAIN (params))
    {
      param_type_list = chainon (param_type_list,
				 build_tree_list (NULL_TREE,
						  TREE_PURPOSE (params)));
    }
  /* Last param if void indicates fixed length list (as opposed to
     printf style va_* list). */
  param_type_list = chainon (param_type_list,
			     build_tree_list (NULL_TREE, void_type_node));

  return build_function_type (return_type, param_type_list);
}

/* PARAM_TYPE_LIST is a tree_list: PURPOSE is a '*_TYPE_NODE' */
tree
create_function_decl (tree function_name,
		      tree param_type_list,
		      tree return_type)
{
  tree function_type = create_function_type (param_type_list,
					     return_type);

  return  build_decl (FUNCTION_DECL, function_name, function_type);
}

tree
create_function_ptr (tree function_name, tree param_type_list, tree return_type)
{
  tree function_decl = create_function_decl (function_name,
					     param_type_list,
					     return_type);
  return build1 (ADDR_EXPR,
		 build_pointer_type (TREE_TYPE (function_decl)),
		 function_decl);
}

/* PARAMS is a tree_list: PURPOSE is a '*_TYPE_NODE'
   VALUE is an 'IDENTIFIER_NODE' */
tree
make_param_list (tree function_decl, tree params)
{
  int i = -1;

  for (; params; params = TREE_CHAIN (params))
    {
      i++;
      /* Store PARAM_DECL in FORMAL_PARAMS to access parameter by number */
      formal_params[i] = build_decl (PARM_DECL,
				     TREE_VALUE (params),
				     TREE_PURPOSE (params));
      /* Some languages have different nominal and real types.  */
      DECL_ARG_TYPE (formal_params[i]) = TREE_TYPE (formal_params[i]);

      /* Set 'FUNCTION_DECL' in which the formal parameter is declared */
      DECL_CONTEXT (formal_params[i]) = function_decl;

      /* Store PARAM_DECL in symbol table entry to access parameter later by name */
      IDENTIFIER_DECL (TREE_VALUE (params)) = formal_params[i];
    }

  for (; i > 0; i--)
    {
      TREE_CHAIN (formal_params[i-1]) = formal_params[i];
    }
  return formal_params[0];
}

tree
make_current_function_decl (tree function_name, tree return_type, tree params)
{
  tree param_list;
  tree function_decl;

  function_decl = create_function_decl (function_name,
 					params,
					return_type);
					
  param_list = make_param_list (function_decl, params);
        
  TREE_PUBLIC (function_decl) = 1;

  DECL_ARGUMENTS (function_decl) = param_list;
        
  DECL_RESULT (function_decl) = build_decl (RESULT_DECL,
					    NULL_TREE,
					    return_type);
					
  DECL_CONTEXT (DECL_RESULT (function_decl)) = function_decl;

  DECL_INITIAL (function_decl) = make_node (BLOCK);

  DECL_SOURCE_FILE (function_decl) = input_filename;
  	
  DECL_SOURCE_LINE (function_decl) = 1;

  announce_function (function_decl);

  allocate_struct_function (function_decl);

  current_function_decl = function_decl;

  current_function_locals = NULL_TREE;

  current_function_body = alloc_stmt_list ();

  return function_decl;
}

tree
create_function_call (tree function_ptr, bool this_check)
{
  tree function_call;
  tree function_args = NULL_TREE;
  tree function_type, param_types;
  int num_of_params;
	
  function_type = TREE_TYPE (TREE_TYPE (function_ptr));
	
  if (TREE_CODE (function_type) != FUNCTION_TYPE)
    {
      internal_error ("create_function_call(): expected tree code "
		      "FUNCTION_TYPE \n");
    }
	
  param_types = TYPE_ARG_TYPES (function_type);
	
  num_of_params = list_length (param_types) - 1;

  /* Create list for function arguments */
  if (num_of_params)
    {
      tree args;
      int i;
      /* Cannot convert stack items in this loop, because the elements
	 of arguments list and parameter list are not in the same
	 order */
      for (i = 0; i < num_of_params; ++i)
	{
	  function_args = tree_cons (NULL_TREE,
				     stack_pop (NULL_TREE),
				     function_args);
	}

      /* Convert arguments to type, specified by the parameters*/
      for (args = function_args; args; args = TREE_CHAIN (args))
	{
	  TREE_VALUE (args) = convert (TREE_VALUE (param_types),
				       TREE_VALUE (args));
						
	  param_types = TREE_CHAIN (param_types);
	}

      if (this_check)
	{
	  TREE_VALUE (function_args) =
	    create_null_reference_check (
					 TREE_VALUE (function_args));
	}
    }

  /* TREE_TYPE (TREE_TYPE (TREE_TYPE(function_ptr))) is return type */
  function_call = build (CALL_EXPR,
			 TREE_TYPE (TREE_TYPE (TREE_TYPE (function_ptr))),
			 function_ptr,
			 function_args,
                         NULL_TREE); /* the static chain argument or NULL */
			       
  TREE_SIDE_EFFECTS (function_call) = 1;

  return function_call;
}

void
finish_function_call (tree function_call)
{
  if (TREE_CODE (TREE_TYPE (function_call)) != VOID_TYPE)
    {
      stack_push (function_call);
    }
  else
    {
      append_to_statement_list (function_call,
				&current_function_body);
    }
}

/*
 * PARAMS: - is a tree list
 *         - PURPOSE is a '*_TYPE_NODE'
 */
void
make_method_call (tree method_name,
		  tree return_type,
		  tree param_type_list,
		  bool this_check)
{
  tree function_ptr, function_call;

  function_ptr = create_function_ptr (method_name,
				      param_type_list,
				      return_type);
					      
  function_call = create_function_call (
					function_ptr,
					this_check);

  finish_function_call (function_call);
}

tree
create_virtual_method_ptr (
			   tree obj_ref,
			   tree return_type,
			   tree param_type_list,
			   int index)
{
  tree field_mono_obj_vtable;
  tree array_type, field_vtable;
  tree function_type, function_ptr_type;

  function_type = create_function_type (param_type_list, return_type);

  function_ptr_type = build_pointer_type (function_type);

  /* create trees for virtual method call */
  
  field_mono_obj_vtable = create_field_access (obj_ref,
					       ptr_mono_vtable_type_node,
					       NULL_TREE,
					       get_identifier("__vtable"));

  array_type = build_array_type (function_ptr_type,
				 build_index_type (integer_zero_node));

  field_vtable = create_field_access (field_mono_obj_vtable,
				      array_type,
				      NULL_TREE,
				      get_identifier("vtable"));

  return build (ARRAY_REF,
		function_ptr_type,
		field_vtable,
		build_int_cst (integer_type_node, index),
    NULL_TREE,
    NULL_TREE);
}

/*
 * INDEX: - is index in virtual method table
 */
void
make_virtual_method_call (tree return_type, tree param_type_list, int index)
{
  int num_of_params = list_length (param_type_list);

  tree obj_ref = stack_elem (num_of_params, NULL_TREE);

  tree function_call = create_function_call (
					     create_virtual_method_ptr (
									obj_ref,
									return_type,
									param_type_list,
									index),
					     true);

  finish_function_call (function_call);
}

tree
create_intf_method_ptr (
        tree obj_ref,
        tree return_type,
        tree param_type_list,
        tree intf_mono_class_name,
        int index)
{
  tree function_type, function_ptr_type;/*, function_ptr, params;*/
  tree mono_obj_vtable;
  tree interface_id, offset, interface_vtable_slot_ptr;
  tree method_ptr;  

  function_type = create_function_type (param_type_list, return_type);

  function_ptr_type = build_pointer_type (function_type);

  /* create trees for interface method call */

  interface_id = build (COMPONENT_REF,
                        uint16_type_node,
                        IDENTIFIER_DECL (intf_mono_class_name),
                        lookup_field_decl(mono_class_type_node,
                              NULL_TREE,
                              get_identifier("interface_id")),
                        NULL_TREE);
                        
  offset = fold (build (MULT_EXPR,
                        unsigned_type_node,
                        TYPE_SIZE_UNIT(ptr_type_node),
                        build (PLUS_EXPR,
                               uint16_type_node,
                               integer_one_node,
                               interface_id)));
                        
  mono_obj_vtable = create_field_access (obj_ref,
                 ptr_mono_vtable_type_node,
                 NULL_TREE,
                 get_identifier("__vtable"));
                 
  interface_vtable_slot_ptr =  build1 (INDIRECT_REF,
                                   build_pointer_type(function_ptr_type),
                                   build (MINUS_EXPR,
                                          build_pointer_type(build_pointer_type(function_ptr_type)),
                                          mono_obj_vtable,
                                          offset));
                                          
  offset = fold (build (MULT_EXPR,
                        unsigned_type_node,
                        build_int_cst (unsigned_type_node, index),
                        TYPE_SIZE_UNIT(ptr_type_node)));
                                          
  method_ptr = build1 (INDIRECT_REF,
                       function_ptr_type,
                       build (PLUS_EXPR,
                              build_pointer_type(function_ptr_type),
                              offset,
                              interface_vtable_slot_ptr));
  
  /*                            
   * following can be used for debugging purposes
   */
  /* 
  params = chainon(NULL_TREE, build_tree_list (TREE_TYPE(obj_ref), NULL_TREE));
  params = chainon(params, build_tree_list (ptr_mono_class_type_node, NULL_TREE));
  params = chainon(params, build_tree_list (integer_type_node, NULL_TREE));

  function_ptr = create_function_ptr (get_identifier ("lookup_intf_method_ptr"),
                                      params,
                                      function_ptr_type);

  stack_push (obj_ref); 
  stack_push (build1 (ADDR_EXPR,
              ptr_mono_class_type_node,
              IDENTIFIER_DECL (intf_mono_class_name)));          
  stack_push (build_int_cst (integer_type_node, index));  
  
  method_ptr = create_function_call(function_ptr, false);
  
  */
    
  return fold(method_ptr);
}

void
make_intf_method_call (
            tree return_type,
            tree param_type_list,
            tree intf_mono_class_name,
            int index)
{
  int num_of_params = list_length (param_type_list);

  tree obj_ref = stack_elem (num_of_params, NULL_TREE);

  tree function_call = create_function_call (
               create_intf_method_ptr (
                  obj_ref,
                  return_type,
                  param_type_list,
                  intf_mono_class_name,
                  index),
               true);

  finish_function_call (function_call);
}

tree
create_null_reference_check (tree ref_of_obj)
{
  tree function_decl, function_ptr, function_call, condition;

  function_decl = create_function_decl (get_identifier ("null_reference"),
					NULL_TREE,
					void_type_node);
  /* This function will not return */
  TREE_THIS_VOLATILE (function_decl) = 1;
	
  function_ptr = build1 (ADDR_EXPR,
			 build_pointer_type (TREE_TYPE (function_decl)),
			 function_decl);

  function_call = create_function_call (function_ptr, false);
  TREE_SIDE_EFFECTS(function_call) = 1;

  /* Make LHS expression be type-correct */
  function_call = build2 (COMPOUND_EXPR, TREE_TYPE (ref_of_obj),
			  function_call, ref_of_obj);
        
  condition = build (EQ_EXPR,
		     boolean_type_node,
		     ref_of_obj,
		     null_pointer_node);

  return build (COND_EXPR,
		TREE_TYPE (ref_of_obj),
		condition,
		function_call,
		ref_of_obj);
}

/*
 * Access a field of a given object
 *
 * OBJ_REF: target object for the field access
 *
 * TYPE: - is a type node
 *       - the type of the field
 * CONTEXT: type which contains original field context (in case of inheritance);
 *          NULL_TREE for built-in fields
 * 
 * FIELD_NAME: - is an identifier node
 *             - name: [assembly name]$[namespace name]$[class name]$[field name]
 */
tree
create_field_access (tree obj_ref, tree type, tree context, tree field_name)
{
  tree decl, obj_record, field_decl;

  decl = TREE_TYPE (TREE_TYPE (obj_ref));

  obj_record = build1 (INDIRECT_REF, decl, obj_ref);

  field_decl = lookup_field_decl (decl, context, field_name);

  return build (COMPONENT_REF, type, obj_record, field_decl, NULL_TREE);
}

tree
create_field_access_ind (tree obj_ref, tree type, int index)
{
  tree decl, obj_record, field_decl;

  decl = TREE_TYPE (TREE_TYPE (obj_ref));

  obj_record = build1 (INDIRECT_REF, decl, obj_ref);

  field_decl = lookup_field_decl_ind (decl, index);

  return build (COMPONENT_REF, type, obj_record, field_decl, NULL_TREE);
}

/*
 * CLASS_NAME: - is an identifier node
 *             - name: [assembly name]$[namespace name]$[class name]
 *
 * FIELD_TYPE: - is a type node
 * 	       - the type of the field
 *
 * FIELD_NAME: - is an identifier node
 *             - name: [field name]
 */
tree
create_static_field_access (tree class_name, tree field_type, tree field_name)
{
  tree class_data_decl = get_class_data_decl (class_name);
	
  tree field_decl = lookup_field_decl (TREE_TYPE (class_data_decl),
				       NULL_TREE, field_name);

  return build (COMPONENT_REF, field_type, class_data_decl, field_decl, NULL_TREE);
}

tree
create_array_elem_access (void)
{
  tree field, elem_type, elem_ref;
  tree first_elem_ref_type, first_elem, first_elem_ref;

  tree index = stack_pop (size_type_node);
	
  tree offset = index;

  tree array_ref = stack_pop (NULL_TREE);
 	
  tree array_type = TREE_TYPE (array_ref);
 	
  tree array_decl = TREE_TYPE (array_type);

  /* create nodes for index out of range check
     (includes ckeck for null reference) */
  tree checked_ref = create_array_bounds_check (array_ref, index);

  /* the field '__vector' the first array element */
  field = lookup_field_decl (array_decl, NULL_TREE, get_identifier("__vector"));
	
  elem_type = TREE_TYPE (field);

  first_elem = create_field_access (checked_ref,
				    elem_type,
				    NULL_TREE,
				    get_identifier("__vector"));
	
  first_elem_ref_type = build_pointer_type (TREE_TYPE (first_elem));
 	
  first_elem_ref = build1 (ADDR_EXPR, first_elem_ref_type, first_elem);

  offset = fold (build (MULT_EXPR,
			TREE_TYPE (index),
			index,
			size_in_bytes (elem_type)));
  elem_ref = fold (build (PLUS_EXPR,
			  first_elem_ref_type,
			  first_elem_ref,
			  offset));

  return build1 (INDIRECT_REF, elem_type, elem_ref);
}

tree
create_array_bounds_check (tree array_ref, tree index)
{
  tree function_decl, function_ptr, function_call, cond1, cond2, check;

  function_decl = create_function_decl (
					get_identifier ("index_out_of_range"),
					NULL_TREE,
					void_type_node);
				
  /* This function will not return */
  TREE_THIS_VOLATILE (function_decl) = 1;

  function_ptr = build1 (ADDR_EXPR,
			 build_pointer_type (TREE_TYPE (function_decl)),
			 function_decl);

  function_call = create_function_call (function_ptr, false);
  TREE_SIDE_EFFECTS(function_call) = 1;

  /* Make type of LHS expression correct */
  function_call = build2 (COMPOUND_EXPR, TREE_TYPE(array_ref),
			  function_call, array_ref);

  cond1 = build (LT_EXPR, boolean_type_node, index, integer_zero_node);

  cond2 = build (GE_EXPR,
		 boolean_type_node,
		 index,
		 get_array_length (array_ref));

  check = build (COND_EXPR,
		 TREE_TYPE (array_ref),
		 cond2,
		 function_call,
		 array_ref);

  check = build (COND_EXPR,
		 TREE_TYPE (check),
		 cond1,
		 function_call,
		 check);

  return check;
}

void
make_assignment (tree lvalue)
{
  tree assignment;
  assignment = build (MODIFY_EXPR,
		      TREE_TYPE (lvalue),
		      lvalue,
		      stack_pop (TREE_TYPE (lvalue)));
			   
  TREE_SIDE_EFFECTS (assignment) = 1;
  TREE_USED (assignment) = 1;
  append_to_statement_list (assignment, &current_function_body);
}

/*
 * TYPE: - is a type node
 *       - the type of the field
 * CONTEXT: - is the type where the original declaration occurred
 *
 * FIELD_NAME: - is an identifier node for the field name
 */
void
make_field_assignment (tree type, tree context, tree field_name)
{
  tree lvalue;
        
  tree rvalue = stack_pop (NULL_TREE);

  /* Create nodes to check null reference of this pointer */
  lvalue = create_null_reference_check (stack_pop (NULL_TREE));

  lvalue = create_field_access (lvalue, type, context, field_name);

  stack_push (rvalue);

  make_assignment (lvalue);
}

/*
 * CLASS_NAME: - is an identifier node
 *             - name: [assembly name]$[namespace name]$[class name]
 *
 * FIELD_TYPE: - is a type node
 * 	       - the type of the field
 *
 * FIELD_NAME: - is an identifier node
 *             - name: [field name]
 */
void
make_static_field_assignment (tree class_name, tree field_type, tree field_name)
{
  tree lvalue = create_static_field_access (class_name,
					    field_type,
					    field_name);

  make_assignment (lvalue);
}



/* TYPE is a '*_TYPE_NODE';  the type of the element of the array */
void
make_array_elem_assignment (void)
{
  tree value = stack_pop (NULL_TREE);

  tree elem_ref = create_array_elem_access ();

  stack_push (value);

  make_assignment (elem_ref);
}

tree
get_array_length (tree array_ref)
{
  /* Create nodes to check null reference of this pointer */
  array_ref = create_null_reference_check (array_ref);

  return create_field_access (array_ref,
			      size_type_node,
			      NULL_TREE,
			      get_identifier("__max_length"));
}

void
make_instr_ldlen (void)
{
  stack_push (get_array_length (stack_pop (NULL_TREE)));
}

void
make_arithmetic_expr (enum tree_code operation)
{
  tree op1, op2;
  enum tree_code t1, t2;

  op2 = stack_pop (NULL_TREE);
  op1 = stack_pop (NULL_TREE);
  t1 = TREE_CODE (TREE_TYPE (op1));
  t2 = TREE_CODE (TREE_TYPE (op2));

  /* TODO: implement and verify the complete operand type table */
  if (t1 == BOOLEAN_TYPE)
    {
      op1 = build1 (CONVERT_EXPR, int32_type_node, op1);
      t1 = INTEGER_TYPE;
    }
  if (t2 == BOOLEAN_TYPE)
    {
      op2 = build1 (CONVERT_EXPR, int32_type_node, op2);
      t2 = INTEGER_TYPE;
    }

  if ((t1 != t2)
      && (TREE_CODE (op1) != INTEGER_CST)
      && (TREE_CODE (op2) != INTEGER_CST))
    {
      internal_error ("different types in arithmetic expression");
    }

  if (t1 == INTEGER_TYPE)
    {
      stack_push (fold (build (operation, TREE_TYPE (op1), op1, op2)));
    }
  else if (t1 == REAL_TYPE)
    {
      stack_push (fold (build (operation, TREE_TYPE (op1), op1, op2)));
    }
  else
    {
      internal_error ("a type of an operand for "
		      "an arithmetic expression is not implemented");
    }
}

void
make_unary_expr (enum tree_code operation)
{
  tree op1;
  enum tree_code t1;

  op1 = stack_pop (NULL_TREE);
  t1 = TREE_CODE (TREE_TYPE (op1));

  /* TODO: implement and verify the complete operand type table */
  if (t1 == BOOLEAN_TYPE)
    {
      op1 = build1 (CONVERT_EXPR, int32_type_node, op1);
      t1 = INTEGER_TYPE;
    }

  if (t1 == INTEGER_TYPE)
    {
      stack_push (fold (build1 (operation, TREE_TYPE (op1), op1)));
    }
  else if (t1 == REAL_TYPE)
    {
      stack_push (fold (build1 (operation, TREE_TYPE (op1), op1)));
    }
  else
    {
      internal_error ("a type of an operand for "
		      "an arithmetic expression is not implemented");
    }
}

/* LABEL_NAME is an 'IDENTIFIER_NODE' */
tree
lookup_label (tree label_name)
{
  tree label_decl;
  label_decl = LABEL_DECL (label_name);

  if (label_decl)
    {
      DECL_CONTEXT (LABEL_DECL (label_name)) = current_function_decl;
    }
  else
    {
      /* Create new 'LABEL_DECL-node' */
      label_decl = build_decl (LABEL_DECL,
			       label_name,
			       void_type_node);
		
      DECL_CONTEXT (label_decl) = current_function_decl;

      /*
       * Store LABEL_DECL in symbol table entry
       * to access label later by name
       */
      LABEL_DECL (label_name) = label_decl;

      /*
       * In LOCAL_LABELS are stored all local label identifiers
       * of the current function for cleanup after
       * rest_of_compilation()
       */
      local_labels = tree_cons (NULL_TREE,
				label_name,
				local_labels);
    }
	
  return label_decl;
}

/* LABEL_NAME is an 'IDENTIFIER_NODE' */
void
make_label (tree label_name)
{
  tree label_decl, label;
  /* Create label if stack is empty */
  if (stack_height () == 0)
    {
      label_decl = lookup_label (label_name);
      label = build1 (LABEL_EXPR, void_type_node, label_decl);
      append_to_statement_list (label, &current_function_body);
    }
}

void
make_branch_on_true (tree label_name)
{
  tree label_decl;
  tree if_true_label, cond;

  label_decl = lookup_label (label_name);

  if_true_label = build1 (GOTO_EXPR, void_type_node, label_decl);

  cond = build3 (COND_EXPR, void_type_node,
		 stack_pop (boolean_type_node),
		 if_true_label, build_empty_stmt());

  append_to_statement_list (cond, &current_function_body);
}

void
make_branch_on_false (tree label_name)
{
  tree label_decl;
  tree if_false_label, cond;

  label_decl = lookup_label (label_name);

  if_false_label = build1 (GOTO_EXPR, void_type_node, label_decl);

  cond = build3 (COND_EXPR, void_type_node,
		 stack_pop (boolean_type_node),
		 build_empty_stmt(), if_false_label);

  append_to_statement_list (cond, &current_function_body);
}

void
make_branch_comparison (enum tree_code condition,
			enum tree_code unordered_condition,
			tree label_name,
			bool unsignedp)
{
  tree op1, op2;
  tree condition_expr, comp;
  tree label_decl, if_true_label;
	
  op2 = stack_pop (NULL_TREE);
  op1 = stack_pop (NULL_TREE);
	
  /* TODO: implement and verify the complete operand type table */

  if (TREE_CODE (TREE_TYPE (op1)) != TREE_CODE (TREE_TYPE (op2)))
    {		internal_error ("different types in branch comparison");
    }

  /* Deal with unsigned/unordered */
  if (unsignedp)
    switch (TREE_CODE (TREE_TYPE (op1))) {
    case INTEGER_TYPE:
      /* Convert integer nodes into unsigned types if requested */
      op1 = convert (cil_unsigned_type (TREE_TYPE (op1)), op1);
      op2 = convert (cil_unsigned_type (TREE_TYPE (op2)), op2);
      break;
    case BOOLEAN_TYPE:
      /* Do no unsigned conversion for booleans */
      break;
    case REAL_TYPE:
      condition = unordered_condition;
      break;
    default:
      internal_error ("unsupported unsigned comparison");
    }

  label_decl = lookup_label (label_name);
        
  /* Get rtx of LABEL_DECL if exits or create new */
  if_true_label = build1 (GOTO_EXPR, void_type_node, label_decl);
        
  /* TODO: support unsigned operands */
  condition_expr = build2 (condition, boolean_type_node, op1, op2);

  comp = build3 (COND_EXPR, void_type_node, condition_expr, 
		 if_true_label,
		 build_empty_stmt ());

  append_to_statement_list (comp, &current_function_body);
}

void
make_branch_unconditional (tree label_name)
{
  tree label_decl, _goto;
  label_decl = lookup_label (label_name);
  _goto = build1 (GOTO_EXPR, void_type_node, label_decl);
  append_to_statement_list (_goto, &current_function_body);
}

/*
 * LABEL_NAMES: - is a tree list
 *         	- PURPOSE is an integer constant
 *              - VALUE is an identifier node, represents the label name
 */

void
make_instr_switch (tree label_names)
{
  tree label, label_decl, value, cases, switch_expr;
  unsigned long index = 0;
	
  tree selector = stack_pop (uint32_type_node);

  cases = make_tree_vec (list_length (label_names) + 1);

  for (label = label_names; label; label = TREE_CHAIN (label))
    {
      label_decl =  build_decl (LABEL_DECL, NULL_TREE, NULL_TREE);

      value = TREE_PURPOSE (label);

      gcc_assert (TREE_TYPE (value) == uint32_type_node);

      label = build (CASE_LABEL_EXPR, void_type_node,
		     value, NULL_TREE, label_decl);

      TREE_VEC_ELT (cases, index) = label;

      ++index;
    }
	
  switch_expr = build (SWITCH_EXPR, selector, NULL_TREE, cases);

  append_to_statement_list (switch_expr, &current_function_body);
}


void
make_return_value (void)
{
  tree return_type, return_value;
        
  return_type = TREE_TYPE (TREE_TYPE (current_function_decl));

  if (stack_height () > 0)
    {
      return_value = stack_pop (return_type);

      return_value = build (MODIFY_EXPR,
			    return_type,
			    DECL_RESULT (current_function_decl),
			    return_value);
				 
      TREE_SIDE_EFFECTS (return_value) = 1;
    }
  else
    {
      return_value = NULL_TREE;
    }

  append_to_statement_list (build1 (RETURN_EXPR, return_type, return_value),
			    &current_function_body);
}

void
make_instr_dup (void)
{
  tree t = stack_pop (NULL_TREE);
  t = save_expr (t);
  stack_push (t);
  stack_push (t);
}

void
make_instr_pop (void)
{
  tree t = stack_pop (NULL_TREE);
  append_to_statement_list (t, &current_function_body);
}

void
make_delegate_ctor (void)
{
  tree target, method_ptr;

  tree delegate_ref = formal_params[0];

  target = create_field_access_ind (delegate_ref,
				    ptr_type_node,
				    MONO_DELEGATE_TARGET);

  method_ptr = create_field_access_ind (delegate_ref,
					ptr_type_node,
					MONO_DELEGATE_METHOD_PTR);

  stack_push (formal_params[1]);
        
  make_assignment (target);
	
  stack_push (convert (ptr_type_node, formal_params[2]));

  make_assignment (method_ptr);
}

void
make_delegate_invoke (void)
{
  tree target, method_ptr, cond;
  tree function_type, return_type, param_types, function_args;
  tree static_call, non_static_call, result;
  int i, num_of_params;

  tree delegate_ref = formal_params[0];

  target = create_field_access_ind (delegate_ref,
				    ptr_type_node,
				    MONO_DELEGATE_TARGET);
	
  function_type = TREE_TYPE (current_function_decl);
	
  method_ptr = create_field_access_ind (delegate_ref,
					build_pointer_type (function_type),
					MONO_DELEGATE_METHOD_PTR);

  cond = build (NE_EXPR, boolean_type_node, target, null_pointer_node);

  /* Part1: method invoke is not static */

  param_types = TYPE_ARG_TYPES (function_type);
	
  TREE_VALUE (param_types) = ptr_type_node;

  num_of_params = list_length (param_types) - 1;

  /* Create list for function arguments */

  function_args = build_tree_list (NULL_TREE, target);

  for (i = 1; i < num_of_params; ++i)
    {
      function_args = chainon (function_args,
			       build_tree_list (NULL_TREE,
						formal_params[i]));
    }
	
  return_type = TREE_TYPE (TREE_TYPE (TREE_TYPE (method_ptr)));

  non_static_call = build (CALL_EXPR,
			   return_type,
			   method_ptr,
			   function_args,
			   NULL_TREE);
		       
  TREE_SIDE_EFFECTS (non_static_call) = 1;
        
  /* Part2: method invoke is static */
	
  function_type = build_function_type (return_type,
				       TREE_CHAIN (param_types));

  function_args = TREE_CHAIN (function_args);
	
  static_call = build (CALL_EXPR,
		       return_type,
		       method_ptr,
		       function_args,
		       NULL_TREE);

  TREE_SIDE_EFFECTS (static_call) = 1;

  result = build3 (COND_EXPR, return_type,
		   cond,
		   non_static_call,
		   static_call);

  if (return_type != void_type_node)
    {

      result = build (MODIFY_EXPR,
		      return_type,
		      DECL_RESULT (current_function_decl),
		      result);
      TREE_SIDE_EFFECTS (result) = 1;

      result = build1 (RETURN_EXPR, void_type_node, result);
      TREE_SIDE_EFFECTS (result) = 1;
    }

  append_to_statement_list (result, &current_function_body);
}

void
compile_function (void)
{
  int i;
  int num_of_params;
  tree label, block, fndecl;

  /* Cleanup old entries in local_vars and in symbol table entries */
  for (i=0; i<num_of_local_vars; i++) {
    IDENTIFIER_DECL (DECL_NAME (local_vars[i])) = NULL_TREE;
    local_vars[i] = NULL_TREE;
  }
  num_of_local_vars = 0;

  /* Cleanup old entries in formal_params and in symbol table entries */
  num_of_params = list_length (DECL_ARGUMENTS (current_function_decl));
        
  for (i=0; i<num_of_params; i++)
    {
      IDENTIFIER_DECL (DECL_NAME (formal_params[i])) = NULL_TREE;
      formal_params[i] = NULL_TREE;
    }
	
  /* Cleanup local_labels and entrys in symbol table */
  label = local_labels;
  for (; label; label = TREE_CHAIN (label) )
    {
      LABEL_DECL (TREE_VALUE (label)) = NULL_TREE;
    }
  local_labels = NULL_TREE;
	
  /* Ensure that the stack is empty */
  while (stack_height () > 0 )
    {
      stack_pop (NULL_TREE);
    }
  block = build_block (current_function_locals, NULL_TREE, 
		       NULL_TREE, NULL_TREE);
  DECL_SAVED_TREE (current_function_decl) = 
    build3 (BIND_EXPR, void_type_node,
	    current_function_locals, 
	    current_function_body, 
	    block);

  cfun->function_end_locus = input_location; /* TODO proper line numbering */

  fndecl = current_function_decl;
  dump_function (TDI_original, fndecl);
  gimplify_function_tree (fndecl);
  dump_function (TDI_generic, fndecl);
  cgraph_node (fndecl);  
  cgraph_finalize_function(fndecl, false);
	
  current_function_decl = NULL_TREE;
  current_function_locals = NULL_TREE;
  current_function_body = NULL_TREE;

  /* generate initialization code, that the cctor is called at startup */

  if (DECL_STATIC_CONSTRUCTOR (fndecl))
    {
      if (targetm.have_ctors_dtors)
	{
	  (* targetm.asm_out.constructor) (
					   XEXP (DECL_RTL (fndecl), 0),
					   DEFAULT_INIT_PRIORITY);
	}
      /*
       * TODO: support calling cctor at startup using
       *       collect2 mechanism
       */
      else
	{
	  internal_error ("calling cctor at startup using"
			  "collect2 mechanism is not supported yet");
	}
    }
	
}

#include "gt-cil-build-trees.h"
