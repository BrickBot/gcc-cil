#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "function.h"
#include "tree-gimple.h"
#include "ggc.h"

#include "scil-builtins.h"
#include "scil-hooks.h"
#include "scil-trees.h"
#include "scil-rt.h"
#include "scil-callbacks.h"

tree scil_bool_type;
tree scil_char_type;

tree scil_object_type;
tree scil_string_type;
tree scil_string_literal_type;

tree scil_float32_type;
tree scil_float64_type;

tree scil_int8_type;
tree scil_int16_type;
tree scil_int32_type;
tree scil_int64_type;

tree scil_native_int_type;
tree scil_native_uint_type;

tree scil_typed_ref;

tree scil_uint8_type;
tree scil_uint16_type;
tree scil_uint32_type;
tree scil_uint64_type;

tree scil_attribute_static;
tree scil_attribute_public;
tree scil_attribute_protected;
tree scil_attribute_private;

static void build_error_mark_node (void);
static void build_integer_nodes (void);
static void build_float_nodes (void);
static void build_size_nodes (void);
static void build_void_nodes (void);
static void build_object_type (void);
static void build_string_type (void);
static void build_main_function (void);

static tree init_type (tree, const char *, const char *);

#define make_scil_builtin_function(name, type, function_code) \
  scil_hook_builtin_function (name, type, function_code, BUILT_IN_FRONTEND, 0, NULL_TREE)

/* see lang_hooks.builtin_function in langhooks.h */
/* reduced copy from c-decl.c */
tree 
scil_hook_builtin_function (const char *name, tree type, int function_code,
                            enum built_in_class bt_class, 
                            const char *library_name, tree attrs)
{
  tree id = get_identifier (name);
  tree decl = scil_build_decl (FUNCTION_DECL, id, type);

  TREE_PUBLIC (decl) = 1;
  DECL_EXTERNAL (decl) = 1;
  DECL_LANG_SPECIFIC (decl) = GGC_CNEW (struct lang_decl);
  DECL_BUILT_IN_CLASS (decl) = bt_class;
  DECL_FUNCTION_CODE (decl) = function_code;
  SCIL_DECL (id) = decl;
  
  if (library_name)
    SET_DECL_ASSEMBLER_NAME (decl, get_identifier (library_name));

  /* Possibly apply some default attributes to this built-in function.  */
  if (attrs)
    decl_attributes (&decl, attrs, ATTR_FLAG_BUILT_IN);
  else
    decl_attributes (&decl, NULL_TREE, 0);

  return decl;
}


/* see scil-builtins.h */
void
scil_append_statement_to_main (tree statement)
{
  /* TODO: get these bits out of the file(s) we are parsing */
  annotate_with_file_line (statement, main_input_filename, 0);

  append_to_statement_list_force (statement,
    &BIND_EXPR_BODY (DECL_SAVED_TREE (SCIL_DECL (main_identifier_node))));
}


/* see scil-builtins.h */
void
scil_make_builtins () 
{
  /* error mark must be the first node to make */
  build_error_mark_node ();

  /* CLS compliant sizetype is signed according to ECMA 335 */
  initialize_sizetypes (true);

  build_integer_nodes ();
  build_size_nodes ();
  build_float_nodes ();
  build_void_nodes ();

  /* used by dbxout.c, dwarf2out.c, and tree_dump.c */
  access_public_node = scil_attribute_public;
  access_protected_node = scil_attribute_protected;
  access_private_node = scil_attribute_private;
  
  build_object_type ();
  build_string_type ();

  /* for builtins that the tree optimizers and the middle end rely upon */
  build_common_builtin_nodes ();

  /* Build explicit main method because some back ends check for "main" */
  build_main_function ();
  
  /* Add runtime specific built-ins */
  scil_rt_make_builtins ();
}

tree
init_type (tree type, const char *name, const char *mangling_chunk)
{
  tree id;

  TYPE_NAME (type) = id = get_identifier (name);
  SCIL_DECL (id) = type;

  TYPE_LANG_SPECIFIC (type) = GGC_CNEW (struct lang_type);
  SCIL_TYPE_MANGLED (type) = get_identifier (mangling_chunk);
  SCIL_NO_COMPRESSION (type) = 1;
  
  return type;
}

void
build_error_mark_node ()
{
  error_mark_node = make_node (ERROR_MARK);
  TREE_TYPE (error_mark_node) = error_mark_node;
}


void
build_integer_nodes ()
{

  scil_int8_type =  init_type (make_signed_type (8), "int8", "a");
  scil_int16_type = init_type (make_signed_type (16), "int16", "s");
  scil_int32_type = init_type (make_signed_type (32), "int32", "l");
  scil_int64_type = init_type (make_signed_type (64), "int64", "x");
  scil_uint8_type =  init_type (make_unsigned_type (8), "uint8", "h");
  scil_uint16_type = init_type (make_unsigned_type (16), "uint16", "t");
  scil_uint32_type = init_type (make_unsigned_type (32), "uint32", "m");
  scil_uint64_type = init_type (make_unsigned_type (64), "uint64", "y");

  switch (INT_TYPE_SIZE)
  {
    case 8: 
      scil_native_int_type = scil_int8_type;
      scil_native_uint_type = scil_uint8_type;
      break;
    case 16: 
      scil_native_int_type = scil_int16_type;
      scil_native_uint_type = scil_uint16_type;
      break;
    case 32: 
      scil_native_int_type = scil_int32_type;
      scil_native_uint_type = scil_uint32_type;
      break;
    case 64: 
      scil_native_int_type = scil_int64_type;
      scil_native_uint_type = scil_uint64_type;
      break;
    default:
      scil_native_int_type = 
        init_type (make_signed_type (INT_TYPE_SIZE), "native int", "i");
      scil_native_uint_type = 
        init_type (make_unsigned_type (INT_TYPE_SIZE), "native uint", "j");
  }

  /* needed by lots of other parts of the GCC */
  integer_type_node = scil_native_int_type;
  integer_zero_node = build_int_cst (integer_type_node, 0);
  integer_one_node = build_int_cst (integer_type_node, 1);
  integer_minus_one_node = build_int_cst (integer_type_node, -1);

  /* ECMA 335: bool occupies 1 byte in memory */
  boolean_type_node = 
  scil_bool_type = init_type (make_unsigned_type (8), "bool", "b");
  TREE_SET_CODE (boolean_type_node, BOOLEAN_TYPE);
  TYPE_MAX_VALUE (boolean_type_node) = build_int_cst (boolean_type_node, 1);
  TYPE_PRECISION (boolean_type_node) = 1;

  boolean_true_node = build_int_cst (scil_bool_type, 1);
  boolean_false_node = build_int_cst (scil_bool_type, 0);


  char_type_node = 
  scil_char_type = init_type (make_signed_type (16), "char", "w");
}

void
build_float_nodes ()
{
  float_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (float_type_node) = 32;
  layout_type (float_type_node);

  double_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (double_type_node) = 64;
  layout_type (double_type_node);

  scil_float32_type = init_type (float_type_node, "float32", "f");
  scil_float64_type = init_type (double_type_node, "float64", "d");
}

void
build_size_nodes ()
{
  /* needed by tree-dfa */
  size_type_node = make_signed_type (POINTER_SIZE);
  set_sizetype (size_type_node);

  /* needed by lots of other parts of the GCC */
  size_zero_node = size_int (0);
  size_one_node = size_int (1);
  bitsize_zero_node = bitsize_int (0);
  bitsize_one_node = bitsize_int (1);
  bitsize_unit_node = bitsize_int (BITS_PER_UNIT);
}

void
build_void_nodes ()
{
  void_type_node = init_type (make_node (VOID_TYPE),"void", "v");
  layout_type (void_type_node);

  void_list_node = build_tree_list (NULL_TREE, void_type_node);

  ptr_type_node = build_pointer_type (void_type_node);
  layout_type (ptr_type_node);

  const_ptr_type_node = build_pointer_type (build_type_variant (void_type_node, 1, 0));
  layout_type (const_ptr_type_node);

  null_pointer_node = build_int_cst (ptr_type_node, 0);
}


void 
build_object_type ()
{
  scil_object_type = 
    scil_register_class (scil_register_assembly ("mscorlib"), "System.Object");
  scil_rt_init_object_type ();
  scil_set_base_class (scil_object_type, NULL_TREE);
  scil_complete_type (scil_object_type);

  /* sets the TYPE_POINTER_TO (scil_object_type) automatically */
  build_pointer_type (scil_object_type);
}


void 
build_string_type ()
{
  scil_string_literal_type = 
    build_array_type (scil_int16_type, build_index_type (integer_zero_node));

  scil_string_type =
    scil_register_class (scil_register_assembly ("mscorlib"), "System.String");
  scil_set_base_class (scil_string_type, scil_object_type);
  scil_rt_init_string_type ();
  scil_complete_type (scil_string_type);
  
  /* sets the TYPE_POINTER_TO (scil_string_type) automatically */
  build_pointer_type (scil_string_type);
}




void
build_main_function ()
{
  main_identifier_node = get_identifier ("main");
  tree char_ptr_ptr_type = build_pointer_type (build_pointer_type (char_type_node));
  tree arg_types = tree_cons (NULL_TREE, integer_type_node, 
                   tree_cons (NULL_TREE, char_ptr_ptr_type, void_list_node));
  tree type = build_function_type (integer_type_node, arg_types);

  tree main_decl = scil_build_decl (FUNCTION_DECL, main_identifier_node, type);
  DECL_EXTERNAL (main_decl) = 0;
  DECL_ARTIFICIAL (main_decl) = 1;
  SCIL_DECL (main_identifier_node) = main_decl;
  DECL_RESULT (main_decl) = scil_build_decl (RESULT_DECL, NULL_TREE, TREE_TYPE (type));
  TREE_PUBLIC (main_decl) = 1;
  allocate_struct_function (main_decl);
  
  /* some code needs to know where the function ends */
  DECL_STRUCT_FUNCTION (main_decl)->function_end_locus = input_location;

  tree block = build_block (NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
  DECL_INITIAL (main_decl) = block;
  DECL_SAVED_TREE (main_decl) = 
    build3 (BIND_EXPR, void_type_node, NULL_TREE, NULL_TREE, NULL_TREE);

  current_function_decl = main_decl;
}

