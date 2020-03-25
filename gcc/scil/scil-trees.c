#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "ggc.h"
#include "vec.h"
#include "toplev.h"

#include "scil-config.h"
#include "scil-hooks.h"
#include "scil-builtins.h"
#include "scil-rt.h"
#include "scil-trees.h"
#include "scil-callbacks.h"

tree
scil_make_record (void)
{
  tree r = make_node (RECORD_TYPE);
  TYPE_LANG_SPECIFIC (r) = GGC_CNEW (struct lang_type);
  return r;
}

tree
scil_get_id (tree node)
{
  return DECL_P (node) ? DECL_NAME (node) : TYPE_NAME (node);
}


/* see scil-trees.h */
tree
scil_get_nested_identifier (tree parent_id, const char *child)
{
  const char *parent_name = IDENTIFIER_POINTER (parent_id);
  int parent_length = IDENTIFIER_LENGTH (parent_id);

  int length = parent_length + strlen (child) + 2; /* + 2 for '.' and \0 */

  char *full_name = (char *) ggc_alloc (length);
  char *current = full_name;

  strcpy (current, parent_name);
  current += parent_length;
  *current++ = '.';
  strcpy (current, child);
  
  return get_identifier_with_length (full_name, length - 1);
}


static tree
make_constructor (tree type)
{ 
  return build_constructor (type, VEC_alloc (constructor_elt, gc, 
                                             list_length (TYPE_FIELDS (type))));
}


void
scil_init_next_field (tree constructor, tree field, tree value)
{
  VEC(constructor_elt,gc) *values = CONSTRUCTOR_ELTS (constructor);

  constructor_elt *element = VEC_quick_push (constructor_elt, values, NULL);
  element->index = field;
  element->value = value;
}


static tree
build_string_literal (const char * content, unsigned int length)
{
  unsigned short* ustr = (unsigned short*) content;
  unsigned int i = 0;

  VEC(constructor_elt,gc) *values = VEC_alloc (constructor_elt, gc, length);

  while (i < length) 
    {
      constructor_elt *element = VEC_quick_push (constructor_elt, values, NULL);
      element->index = build_int_cst (scil_int32_type, i);
      element->value = build_int_cst (scil_int16_type, *(ustr+i) );
      ++i;
    }
  
  return build_constructor (scil_string_literal_type, values);
}

tree
scil_make_string_constant (tree id, const char *content, unsigned int length)
{
  tree decl, field, obj_ctor, string_ctor;

  decl = scil_build_decl (VAR_DECL, id, scil_string_type);
  SCIL_DECL (id) = decl;
  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;
  TREE_ADDRESSABLE (decl) = 1;

  field = TYPE_FIELDS (scil_string_type);

  obj_ctor = make_constructor (scil_object_type);
  scil_rt_init_constant_object (obj_ctor, 
                                TYPE_FIELDS (scil_object_type), 
                                scil_string_type);

  string_ctor = make_constructor (scil_string_type);
  scil_init_next_field (string_ctor, field, obj_ctor);
  scil_rt_init_constant_string (string_ctor, TREE_CHAIN (field), 
                                build_string_literal (content, length), length);

  DECL_INITIAL (decl) = string_ctor;
  TREE_TYPE (decl) = scil_string_type;

  rest_of_decl_compilation (decl, 1, 0);

  return decl;
}

tree 
scil_build_decl (enum tree_code code, tree id, tree type)
{
  tree decl = build_decl (code, id, type);
  DECL_LANG_SPECIFIC (decl) = GGC_CNEW (struct lang_decl);
  return decl;
}

static tree
make_array_constant (tree id, tree element_type, VEC(constructor_elt,gc) *values, unsigned long length)
{
  tree index_type, array_type, constructor, decl;

  element_type = build_type_variant (element_type, 1, 0);
  index_type = build_index_type (build_int_cst (scil_int32_type, length));
  array_type = build_array_type (element_type, index_type);

  constructor = build_constructor (array_type, values);
  decl = scil_build_decl (VAR_DECL, id, array_type);
  SCIL_DECL (id) = decl;
  DECL_INITIAL (decl) = constructor;
  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;
  TREE_ADDRESSABLE (decl) = 1;

  return decl;
}

tree
scil_make_array_constant (tree id, tree element_type, unsigned int length)
{
  return make_array_constant (id, element_type, VEC_alloc (constructor_elt, gc, length), length);
}

tree
scil_make_array_constant_with_init (tree id, tree element_type, unsigned int length, tree source)
{
  VEC(constructor_elt,gc) *values;

  values = VEC_copy (constructor_elt, gc, CONSTRUCTOR_ELTS (DECL_INITIAL (source)));
  VEC_reserve (constructor_elt, gc, values, -length);

  return make_array_constant (id, element_type, values, length);
}

unsigned int
scil_get_array_constant_length (tree array)
{
  return VEC_length (constructor_elt, CONSTRUCTOR_ELTS (DECL_INITIAL (array)));
}

void
scil_set_array_constant_element (tree array, unsigned int index, tree value)
{
  constructor_elt *element;
  VEC(constructor_elt,gc) *values;

  values = CONSTRUCTOR_ELTS (DECL_INITIAL (array));
  element = VEC_index (constructor_elt, values, index);
  element->value = value;
}


unsigned int
scil_add_array_constant_element (tree array)
{
  constructor_elt *element;
  unsigned int index;
  VEC(constructor_elt,gc) *values;

  values = CONSTRUCTOR_ELTS (DECL_INITIAL (array));

  index = VEC_length (constructor_elt, values);
  element = VEC_quick_push (constructor_elt, values, NULL);
  element->index = build_int_cst (scil_int32_type, index);

  return index;
}

tree
scil_base_class (tree type)
{
  tree field = TYPE_FIELDS (type);
  
  if (field == NULL_TREE)
    return NULL_TREE;

  if (strcmp (IDENTIFIER_POINTER (DECL_NAME (field)), SCIL_ID_BASE_CLASS))
    return NULL_TREE;
  
  return TREE_TYPE (field);
}

/* Returns true if TYPE1 is a subclass of TYPE2 or TYPE1 is TYPE2.
*/
bool 
scil_derives_from (tree type1, tree type2)
{
  tree base_type;

  if (type1 == type2)
    return true;
  
  base_type = scil_base_class (type1);
  
  if (base_type == NULL_TREE)
    return false;
  
  return scil_derives_from (base_type, type2);
}


void
scil_add_attribute (tree item, const char *attr_name)
{
  tree attr;

  attr = get_identifier (attr_name);
  attr = build_tree_list (attr, NULL_TREE);

  if (TYPE_P (item)) 
    {
      TYPE_ATTRIBUTES (item) = chainon (TYPE_ATTRIBUTES (item), attr);
    }
  else if (DECL_P (item))
    {
      DECL_ATTRIBUTES (item) = chainon (DECL_ATTRIBUTES (item), attr);
    }
  else
    {
      internal_error ("Attempt to add an attribute to something else than a type or a declaration.");
    }
}
