/*
  CIL Compiler

  Declarations for interfacing to process.c

  Copyright (C) 2003  Jan Möller

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

#ifndef CIL_BUILD_TREES_H
#define CIL_BUILD_TREES_H

/* cil-decl.h includes: config.h system.h tree.h */
#include "cil-decl.h"
#include "il_program.h"

void add_local_vars (tree local_vars);

tree add_field (tree record, tree type, tree context, tree name);

void compile_function (void);

tree create_array_elem_access (void);

tree create_mono_class_decl (tree mono_class_name);

tree create_value_decl (void);

tree create_class_decl (ILClass *klass, tree class_name);

tree create_array_decl (tree elem_type);

tree create_field_access (tree obj_ref, tree type, tree context, tree field_name);

tree create_function_ptr (tree function_name, tree param_type_list, tree return_type);

tree create_null_reference_check (tree ref_of_obj);

tree create_static_field_access (tree class_name, tree field_type, tree field_name);

tree create_static_fields_decl (ILClass*, tree class_name);

tree create_virtual_method_ptr (
				tree obj_ref,
				tree return_type,
				tree param_type_list,
				int index);
        
tree create_intf_method_ptr (
        tree obj_ref,
        tree return_type,
        tree param_type_list,
        tree intf_mono_class_name,
        int index);

void finish_class (tree class_name);

tree get_mono_class_from_ILClass (ILClass *klass);

void make_assignment (tree var);

void make_array_elem_assignment (void);

void make_unary_expr (enum tree_code operation);

void make_arithmetic_expr (enum tree_code operation);

void make_branch_comparison (enum tree_code condition,
			    enum tree_code unordered_condition,
			    tree label_name,
			    bool unsignedp);

void make_branch_on_false (tree label_name);

void make_branch_on_true (tree label_name);

void make_branch_unconditional (tree label_name);

tree make_complex_identifier (tree ident_1, ...);

tree make_current_function_decl (tree function_name, tree return_type, tree params);

void make_delegate_ctor (void);

void make_delegate_invoke (void);

void make_ext_mono_class (tree mono_class_name);

void make_field_assignment (tree type, tree context, tree field_name);

void make_method_call (tree method_name,
		       tree return_type,
		       tree param_type_list,
		       bool this_check);

void make_mono_class_runtime_info (tree mono_class_runtime_info_name, tree mono_vtable_name);

void make_instr_dup (void);

void make_instr_ldlen (void);

void make_instr_pop (void);

void make_instr_switch (tree label_names);

void make_intf_mono_class (tree mono_class_name, tree intf_name_list, int method_count);

void make_label (tree label_name);

void make_new_array (tree elem_type);

void make_new_object (tree class_name, tree ctor_name, tree ctor_params);

void make_mono_class (tree mono_class_name, tree intf_name_list, tree parent_name,
										 int method_count, int instance_size, int vtable_size,
                      tree mono_class_runtime_info_name);

void make_mono_vtable (tree class_name, tree vt_name, tree vtable_entries);

void make_return_value (void);

void make_static_field_assignment (tree class_name,
				   tree field_type,
				   tree field_name);

void make_virtual_method_call (tree return_type, tree param_type_list, int index);
void make_intf_method_call (tree return_type,
            tree param_type_list,
            tree intf_mono_class_name,
            int index);

void make_string (tree name, const char* str, unsigned long len);

#endif  /* CIL_BUILD_TREES_H */

