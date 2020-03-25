/*
  CIL Compiler

  Process declarations and variables

  Copyright (C) 2003, 2004 Jan Möller

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

/* cil-decl.h includes: config.h system.h tree.h */
#include "cil-decl.h"

/* GCC headers. */
#include "coretypes.h"
#include "tm.h"
#include "ggc.h"
#include "langhooks-def.h"
#include "langhooks.h"
#include "errors.h"
#include "debug.h"
#include "convert.h"
#include "toplev.h"

#include "cil.h"
#include "mono_structs.h"

/* global variables; see cil-decl.h */
tree cil_global_trees[CILTI_MAX];
int num_of_local_vars = 0;
tree local_vars[100];
tree formal_params[10];
tree local_labels = NULL_TREE;

/* local functions needed by LANG_HOOKS */
static bool
cil_mark_addressable (tree expr);
static tree
cil_signed_type (tree type);
static tree
cil_signed_or_unsigned_type (int unsignedp, tree type);
static tree
cil_type_for_mode (enum machine_mode mode, int unsignedp);
static tree
cil_type_for_size (unsigned bits, int unsignedp);

static int
global_bindings_p (void);
static void
insert_block (tree block);
static tree
pushdecl (tree x);
static tree
getdecls (void);

/* Tree code type/name/code tables.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) TYPE,

const enum tree_code_class tree_code_type[] = {
#include "tree.def"
  tcc_exceptional
};
#undef DEFTREECODE

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) LENGTH,

const unsigned char tree_code_length[] = {
#include "tree.def"
  0
};
#undef DEFTREECODE

#define DEFTREECODE(SYM, NAME, TYPE, LEN) NAME,

const char *const tree_code_name[] = {
#include "tree.def"
  "@@dummy"
};
#undef DEFTREECODE

/* Define LANG_HOOKS */

#undef LANG_HOOKS_NAME
#define LANG_HOOKS_NAME	"GNU cil"
#undef LANG_HOOKS_INIT
#define LANG_HOOKS_INIT			cil_init
#undef LANG_HOOKS_PARSE_FILE
#define LANG_HOOKS_PARSE_FILE		cil_parse_file
#undef LANG_HOOKS_FINISH
#define LANG_HOOKS_FINISH		cil_finish

#undef LANG_HOOKS_INIT_OPTIONS
#define LANG_HOOKS_INIT_OPTIONS		cil_init_options
#undef LANG_HOOKS_HANDLE_OPTION
#define LANG_HOOKS_HANDLE_OPTION	cil_handle_option

#undef LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION
#define LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION tree_rest_of_compilation


/* Define required LANG_HOOKS */

#undef LANG_HOOKS_MARK_ADDRESSABLE
#define LANG_HOOKS_MARK_ADDRESSABLE		cil_mark_addressable
#undef LANG_HOOKS_SIGNED_TYPE
#define LANG_HOOKS_SIGNED_TYPE			cil_signed_type
#undef LANG_HOOKS_UNSIGNED_TYPE
#define LANG_HOOKS_UNSIGNED_TYPE		cil_unsigned_type
#undef LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE
#define LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE	cil_signed_or_unsigned_type
#undef LANG_HOOKS_TYPE_FOR_MODE
#define LANG_HOOKS_TYPE_FOR_MODE		cil_type_for_mode
#undef LANG_HOOKS_TYPE_FOR_SIZE
#define LANG_HOOKS_TYPE_FOR_SIZE		cil_type_for_size

bool
cil_mark_addressable (tree expr)
{
  switch (TREE_CODE (expr))
    {
    case COMPONENT_REF:
      break;
    case VAR_DECL:
    case FUNCTION_DECL:
      TREE_ADDRESSABLE (expr) = 1;
      return true;
    default:
      internal_error ("function 'cil_mark_addressable(tree expr)': TREE_CODE of expr not implemented");
    }
  return true;
}

/* Return a signed type the same as TYPE in other respects. */
tree
cil_signed_type (tree type)
{
  return cil_signed_or_unsigned_type (0, type);
}

/* Return an unsigned type the same as TYPE in other respects. */
tree
cil_unsigned_type (tree type)
{
  return cil_signed_or_unsigned_type (1, type);
}

tree
cil_signed_or_unsigned_type (int unsignedp, tree type)
{
  if (TREE_CODE (type) == INTEGER_TYPE
      && CIL_TYPE_NATIVE (type))
    {
      return unsignedp ? native_uint_type_node : native_int_type_node;
    }

  if (TYPE_PRECISION (type) == 8)
    {
      return unsignedp ? uint8_type_node : int8_type_node;
    }
	
  if (TYPE_PRECISION (type) == 16)
    {
      return unsignedp ? uint16_type_node : int16_type_node;
    }
	
  if (TYPE_PRECISION (type) == 32)
    {
      return unsignedp ? uint32_type_node : int32_type_node;
    }
	
  if (TYPE_PRECISION (type) == 64)
    {
      return unsignedp ? uint64_type_node : int64_type_node;
    }
	
  internal_error ("cil_signed_or_unsigned_type(): type not implemented");
}

/* Return a data type that has machine mode MODE.
   If the mode is an integer,
   then UNSIGNEDP selects between signed and unsigned types.  */

tree
cil_type_for_mode (enum machine_mode mode, int unsignedp)
{
  if (mode == TYPE_MODE (int8_type_node))
    return unsignedp ? uint8_type_node : int8_type_node;
  if (mode == TYPE_MODE (int16_type_node))
    return unsignedp ? uint16_type_node : int16_type_node;
  if (mode == TYPE_MODE (int32_type_node))
    return unsignedp ? uint32_type_node : int32_type_node;
  if (mode == TYPE_MODE (int64_type_node))
    return unsignedp ? uint64_type_node : int64_type_node;
  if (mode == TYPE_MODE (float_type_node))
    return float_type_node;
  if (mode == TYPE_MODE (double_type_node))
    return double_type_node;

  internal_error ("function cil_type_for_mode(): type not implemented");
}

tree
cil_type_for_size (unsigned bits, int unsignedp)
{
  if (bits == TYPE_PRECISION (int8_type_node))
    {
      return unsignedp ? uint8_type_node : int8_type_node;
    }
	
  if (bits == TYPE_PRECISION (int16_type_node))
    {
      return unsignedp ? uint16_type_node : int16_type_node;
    }

  if (bits == TYPE_PRECISION (int32_type_node))
    {
      return unsignedp ? uint32_type_node : int32_type_node;
    }

  if (bits == TYPE_PRECISION (int64_type_node))
    {
      return unsignedp ? uint64_type_node : int64_type_node;
    }

  internal_error ("function cil_type_for_size(): size not implemented");
}

/* Required functions; needed by LANG_HOOKS */

int
global_bindings_p (void)
{
  return 1;
}

void
insert_block (tree block ATTRIBUTE_UNUSED)
{
  internal_error ("function 'void insert_block (tree block)' not implemented");
}

tree
pushdecl (tree x ATTRIBUTE_UNUSED)
{
  internal_error ("function 'tree pushdecl (tree x)' not implemented");
}

tree
getdecls (void)
{
  return NULL_TREE;
}

/* Copied from c-decl.c. Invoked in i386.c */

/* Return a definition for a builtin function named NAME and whose data type
   is TYPE.  TYPE should be a function type with argument types.
   FUNCTION_CODE tells later passes how to compile calls to this function.
   See tree.h for its possible values.

   If LIBRARY_NAME is nonzero, use that for DECL_ASSEMBLER_NAME,
   the name to be called if we can't opencode the function.  If
   ATTRS is nonzero, use that for the function's attribute list.  */

/* Copied from c-decl.c. Invoked in i386.c */

tree
builtin_function (const char *name, tree type, int function_code, 
		  enum built_in_class class, const char* library_name, 
		  tree attrs)
{
  tree decl = build_decl (FUNCTION_DECL, get_identifier (name), type);
  DECL_EXTERNAL (decl) = 1;
  TREE_PUBLIC (decl) = 1;
  if (library_name)
    SET_DECL_ASSEMBLER_NAME (decl, get_identifier (library_name));
  make_decl_rtl (decl);
  pushdecl (decl);
  DECL_BUILT_IN_CLASS (decl) = class;
  DECL_FUNCTION_CODE (decl) = function_code;

  /* Possibly apply some default attributes to this built-in function.  */
  if (attrs)
    decl_attributes (&decl, attrs, ATTR_FLAG_BUILT_IN);
  else
    decl_attributes (&decl, NULL_TREE, 0);

  return decl;
}
/* Set required LANG_HOOKS */
const struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

/* Required non-static functions; needed by gcc */

tree
poplevel (int keep ATTRIBUTE_UNUSED,
	  int reverse ATTRIBUTE_UNUSED,
	  int functionbody ATTRIBUTE_UNUSED)
{
  internal_error ("function 'tree poplevel (int keep, int reverse, int functionbody)' not implemented");
}

/* Create an expression whose value is that of EXPR,
   converted to type TYPE.  The TREE_TYPE of the value
   is always TYPE.  This function implements all reasonable
   conversions; callers should filter out those that are
   not permitted by the language being compiled.  */
   
tree
convert (tree type, tree expr)
{
  enum tree_code code = TREE_CODE (type);

  if (code == INTEGER_TYPE)
    {
      return fold (convert_to_integer (type, expr));
    }

  if (code == BOOLEAN_TYPE)
    {
      tree other;
      switch (TREE_CODE (TREE_TYPE (expr)))
	{
	case INTEGER_TYPE:
	  other = integer_zero_node;
	  break;
	case POINTER_TYPE:
	  other = null_pointer_node;
	  break;
	case BOOLEAN_TYPE:
	  return expr;
	default:
	  internal_error ("convert to bool: TREE_CODE not implemented");
	}
      return fold (build (NE_EXPR, boolean_type_node, expr, other));
    }

  if (code == POINTER_TYPE)
    {
      return fold (convert_to_pointer (type, expr));
    }

  if (code == REAL_TYPE)
    {
      return fold (convert_to_real (type, expr));
    }

  if (code == RECORD_TYPE)
    {
      if (TREE_TYPE(expr) == type)
	return expr;
      internal_error("convert(): unsupported struct conversion");
    }

  internal_error ("function convert(): conversion not implemented");
}


/* Set required global variables */

void
cil_init_decl_processing (void)
{
  current_function_decl = NULL_TREE;

  build_common_tree_nodes (1, 0); /* signed char, unsigned size type (default in previous versions) */

  size_type_node = make_unsigned_type (POINTER_SIZE);
  set_sizetype (size_type_node);

  build_common_tree_nodes_2 (0); /* double is long */

#if CHAR_TYPE_SIZE == 8
  int8_type_node = signed_char_type_node;
  uint8_type_node = unsigned_char_type_node;
#else
#error "could not find int8 type"
#endif

#if SHORT_TYPE_SIZE == 16
  int16_type_node = short_integer_type_node;
  uint16_type_node = short_unsigned_type_node;
#else
#error "could not find int16 type"
#endif

#if INT_TYPE_SIZE == 32
  int32_type_node = integer_type_node;
  uint32_type_node = unsigned_type_node;
#elif LONG_TYPE_SIZE == 32
  int32_type_node = long_integer_type_node;
  uint32_type_node = long_unsigned_type_node;
#else
#error "could not find int32 type"
#endif

#if LONG_LONG_TYPE_SIZE == 64
  int64_type_node = long_long_integer_type_node;
  uint64_type_node = long_long_unsigned_type_node;
#else
#error "could not find int64 type"
#endif

  native_int_type_node = copy_node (integer_type_node);
  CIL_TYPE_NATIVE (native_int_type_node) = 1;
	
  native_uint_type_node = copy_node (unsigned_type_node);
  CIL_TYPE_NATIVE (native_uint_type_node) = 1;

  cil_char_type_node = uint16_type_node;


	mono_class_runtime_info_type_node = create_struct_mono_class_runtime_info();
	
	/* create struct member_info for MonoClass (mono_class_member_info)
	 * e.g. field, method, property, event */
	mono_class_member_info_type_node = create_struct_mono_class_member_info();
	
  /* Create struct MonoClass */
  mono_class_type_node = make_node (RECORD_TYPE);
  
  /* create pointer to MonoClass before creating MonoClass struct
   * because MonoClass has fields of type pointer to MonoClass */
  ptr_mono_class_type_node = build_pointer_type (mono_class_type_node);
  mono_class_type_node = add_mono_class_fields(mono_class_type_node);
  layout_type (mono_class_type_node);
    	
  /* Create struct MonoVTable */
  mono_vtable_type_node = create_struct_mono_vtable ();
  ptr_mono_vtable_type_node = build_pointer_type (mono_vtable_type_node);

  /* Create MonoString */
  mono_string_type_node = make_node (RECORD_TYPE);
  /* XXX eliminate string here */
  add_mono_obj_fields (mono_string_type_node);
  add_mono_string_fields (mono_string_type_node);
  layout_type (mono_string_type_node);
}

#include "gtype-cil.h"
