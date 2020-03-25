/*
  CIL Compiler

  Declarations for interfacing to cil-decl.c

  Copyright (C) 2003 Jan Möller

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

#ifndef CIL_DECL_H
#define CIL_DECL_H

/* GCC headers. */
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

/* PNet headers */
#include "pnet.h"

/* list of library paths for searching assemblies */
extern const char **libdirs;
extern int num_libdirs;

/* TODO: support more than 100 local variables */
/* In LOCAL_VARS are stored all local variables of the current function
   to access them later by number */
extern int num_of_local_vars;
/* No GGC-Root, beause 'VAR_DECL-nodes' are stored in symbol table */
extern tree local_vars[100];

/* TODO: support more than 10 formal parameters */
/* In FORMAL_PARAMS are stored all parameters of the current function
   to access them later by number */
/* No GGC-Root, beause 'PARM_DECL-nodes' are stored in symbol table */
extern tree formal_params[10];

/* In LOCAL_LABELS (a TREE_LIST) are stored all local label
   identifiers of the current function for cleanup after rest_of_compilation() */
extern tree local_labels;

/* NODE must be an IDENTIFIER_NODE */
#define IDENTIFIER_DECL(NODE) \
( (struct lang_identifier *) (NODE) )->var_or_param_or_func_decl
#define LABEL_DECL(NODE) \
( (struct lang_identifier *) (NODE) )->label_decl
#define IDENTIFIER_CLASS_DECL(NODE) \
( (struct lang_identifier *) (NODE) )->class_decl
#define IDENTIFIER_STATIC_FIELDS_DECL(NODE) \
( (struct lang_identifier *) (NODE) )->static_fields_decl

#define TYPE_ILClass(NODE) (cil_type_lang_specific (NODE) -> klass)
#define TYPE_ARRAYTYPE(NODE) (cil_type_lang_specific (NODE) -> array_type)
#define TYPE_VALUETYPE(NODE) (cil_type_lang_specific (NODE) -> value_type)
#define TYPE_OBJECTTYPE(NODE) (cil_type_lang_specific (NODE) -> object_type)

struct lang_identifier GTY(())
{
  struct tree_identifier identifier;
  tree var_or_param_or_func_decl;
  tree label_decl;
  tree class_decl;
  tree static_fields_decl;
};

/* The resulting tree type.  */

union lang_tree_node
GTY((desc ("TREE_CODE (&%h.generic) == IDENTIFIER_NODE"),
     chain_next ("(union lang_tree_node *)TREE_CHAIN (&%h.generic)")))
{
  union tree_node GTY((tag ("0"),
		       desc ("tree_node_structure (&%h)")))
    generic;
  struct lang_identifier GTY ((tag ("1"))) identifier;
};

struct lang_decl GTY(())
{
  int unused;
};

struct lang_type GTY(())
{
  void* GTY((skip(""))) klass; /* for classes */
  tree array_type;
  tree value_type;
  tree object_type;
};
struct lang_type *cil_type_lang_specific(tree t);

struct language_function GTY(())
{
  int unused;
};

enum cil_tree_index
  {
    CILTI_INT8_TYPE_NODE,
    CILTI_UINT8_TYPE_NODE,
    CILTI_INT16_TYPE_NODE,
    CILTI_UINT16_TYPE_NODE,
    CILTI_INT32_TYPE_NODE,
    CILTI_UINT32_TYPE_NODE,
    CILTI_NATIVE_INT_TYPE_NODE,
    CILTI_NATIVE_UINT_TYPE_NODE,
    CILTI_CHAR_TYPE_NODE,
    CILTI_MONO_VTABLE_TYPE_NODE,
    CILTI_PTR_MONO_VTABLE_TYPE_NODE,
    CILTI_MONO_CLASS_TYPE_NODE,
    CILTI_PTR_MONO_CLASS_TYPE_NODE,
    CILTI_MONO_CLASS_RUNTIME_INFO_TYPE_NODE,
    CILTI_MONO_CLASS_MEMBER_INFO_TYPE_NODE,
    CILTI_MONO_STRING_TYPE_NODE,
    CILTI_INT64_TYPE_NODE,
    CILTI_UINT64_TYPE_NODE,
    CILTI_MAX
  };

extern GTY(()) tree cil_global_trees[CILTI_MAX];

/* Language-specific flags */

/*
 * In an INTEGER_TYPE nonzero if this type
 * is 'native int' or 'native unsigned int'
 */
#define CIL_TYPE_NATIVE(TYPE) TYPE_LANG_FLAG_0 (TYPE)


#define int8_type_node \
	cil_global_trees[CILTI_INT8_TYPE_NODE]
#define uint8_type_node \
	cil_global_trees[CILTI_UINT8_TYPE_NODE]
#define int16_type_node \
	cil_global_trees[CILTI_INT16_TYPE_NODE]
#define uint16_type_node \
	cil_global_trees[CILTI_UINT16_TYPE_NODE]
#define int32_type_node \
	cil_global_trees[CILTI_INT32_TYPE_NODE]
#define uint32_type_node \
	cil_global_trees[CILTI_UINT32_TYPE_NODE]
#define native_int_type_node \
	cil_global_trees[CILTI_NATIVE_INT_TYPE_NODE]
#define native_uint_type_node \
	cil_global_trees[CILTI_NATIVE_UINT_TYPE_NODE]
#define cil_char_type_node \
	cil_global_trees[CILTI_CHAR_TYPE_NODE]
#define mono_vtable_type_node \
	cil_global_trees[CILTI_MONO_VTABLE_TYPE_NODE]
#define ptr_mono_vtable_type_node \
	cil_global_trees[CILTI_PTR_MONO_VTABLE_TYPE_NODE]
#define mono_class_type_node \
	cil_global_trees[CILTI_MONO_CLASS_TYPE_NODE]
#define ptr_mono_class_type_node \
	cil_global_trees[CILTI_PTR_MONO_CLASS_TYPE_NODE]
#define mono_class_runtime_info_type_node \
	cil_global_trees[CILTI_MONO_CLASS_RUNTIME_INFO_TYPE_NODE]
#define mono_class_member_info_type_node \
	cil_global_trees[CILTI_MONO_CLASS_MEMBER_INFO_TYPE_NODE]
#define mono_string_type_node \
	cil_global_trees[CILTI_MONO_STRING_TYPE_NODE]
#define int64_type_node \
	cil_global_trees[CILTI_INT64_TYPE_NODE]
#define uint64_type_node \
	cil_global_trees[CILTI_UINT64_TYPE_NODE]
	
tree
builtin_function (const char *name, tree type, int function_code, 
		  enum built_in_class class, const char* library_name, 
		  tree attrs);

/* Required non-static functions; needed by gcc */
tree poplevel (int keep, int reverse, int functionbody);

tree convert (tree type, tree expr);

/* Used by cil_init() in cil1.c */
void cil_init_decl_processing(void);


tree cil_unsigned_type (tree type);

#endif  /* CIL-DECL_H  */
