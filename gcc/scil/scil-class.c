/* The CIL front end callbacks for building classes. 
 */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "toplev.h"
#include "cgraph.h"

#include "scil-config.h"
#include "scil-hooks.h"
#include "scil-callbacks.h"
#include "scil-rt.h"
#include "scil-trees.h"
#include "scil-builtins.h"
#include "scil-mangle.h"


/* see scil-callbacks.h */
tree
scil_register_assembly (const char *name)
{
  tree id = get_identifier (name);
  if (!SCIL_DECL (id))
    {
      tree decl = scil_build_decl (NAMESPACE_DECL, id, void_type_node);
      SCIL_DECL (id) = decl;
      SCIL_DECL_UNQUALIFIED (decl) = get_identifier (name);
    }
  return SCIL_DECL (id);
}

static tree
get_namespace_identifier (tree container, const char *name_space)
{
  tree id = scil_get_nested_identifier (DECL_NAME (container), name_space);
  if (!SCIL_DECL (id))
    {
      tree decl = scil_build_decl (NAMESPACE_DECL, id, void_type_node);
      SCIL_DECL (id) = decl;
      SCIL_DECL_UNQUALIFIED (decl) = get_identifier (name_space);
      DECL_CONTEXT (decl) = container;
    }
  return id;
}

/* see scil-callbacks.h */
tree
scil_register_namespace (tree container, const char *name_space)
{
  if (!name_space || !name_space[0])
    return container;

  /* find first dot in namespace */
  int count = 0;
  const char *next_part = name_space;
  while (*next_part && *next_part != '.')
    {
      next_part++;
      count++;
    }

  /* get full identifier for that namespace and set decls if necessary*/
  tree id;
  if (*next_part)
    {
      char *buf = alloca (count + 1);
      memcpy (buf, name_space, count);
      buf [count] = '\0';
      id = get_namespace_identifier (container, buf);
      
      /* jump over '.' */
      ++next_part;
    }
  else
    {
      id = get_namespace_identifier (container, name_space);
    }
  
  return scil_register_namespace (SCIL_DECL (id), next_part);
}


/* see scil-callbacks.h */
tree 
scil_register_class (tree container, const char *name)
{ 
  int length;
  tree c, id, type;
 
  /* find last dot in name */
  const char *class_name = name;
  const char *pos = name;
  while (*pos)
    {
      if (*pos++ == '.')
        {
          class_name = pos;
        }
    }

  /* register namespace if necessary and get container ID */
  length = (int) class_name - (int) name;
  if (length > 1)
    {
      char *buf = alloca (length);
      memcpy (buf, name, --length);
      buf [length] = '\0';
      c = scil_register_namespace (container, buf);
    }
  else
    {
      c = container;
    }
  
  /* register class if necessary */
  id = scil_get_nested_identifier (scil_get_id (c), class_name);
  type = SCIL_DECL (id);
  if (type == NULL_TREE)
    {
      type = scil_make_record ();
      TYPE_NAME (type) = id;
      TYPE_CONTEXT (type) = c;
      TYPE_NEEDS_CONSTRUCTING (type) = 1;
      SCIL_TYPE_UNQUALIFIED (type) = get_identifier (class_name);
      SCIL_DECL (id) = type;
    }
  
  /* to make POINTER_TO available */
  build_pointer_type (type);
  
  return type;
}


/* Makes a deep copy of BASE_BINFO and attaches it to BINFO.*/
static void
copy_base_binfo (tree base_binfo, tree binfo)
{
  tree new_base_binfo;

  if (base_binfo == NULL_TREE)
    return;

  new_base_binfo = make_tree_binfo (BINFO_N_BASE_BINFOS (base_binfo));
  
  BINFO_TYPE (new_base_binfo) = BINFO_TYPE (base_binfo);
  BINFO_OFFSET (new_base_binfo) = BINFO_OFFSET (base_binfo);
  BINFO_VIRTUALS (new_base_binfo) = BINFO_VIRTUALS (base_binfo);
  BINFO_VTABLE (new_base_binfo) = BINFO_VTABLE (base_binfo);
  BINFO_BASE_ACCESSES (new_base_binfo) = BINFO_BASE_ACCESSES (base_binfo);

  /* Recursively copy base binfo of BINFO.  */
  if (BINFO_N_BASE_BINFOS (base_binfo))
    {
      copy_base_binfo (BINFO_BASE_BINFO (base_binfo, 0), new_base_binfo);
    }

  BINFO_INHERITANCE_CHAIN (new_base_binfo) = binfo;
  BINFO_BASE_APPEND (binfo, new_base_binfo);
}


void
scil_set_base_class (tree type, tree base_type)
{
  
  tree field, binfo;
     

  /* don't replace existing base class definition  */
  if (TYPE_BINFO (type) != NULL_TREE)
    return;

  TYPE_BINFO (type) = binfo = make_tree_binfo (base_type == NULL_TREE ? 0 : 1);
  BINFO_TYPE (binfo) = type;
  BINFO_OFFSET (binfo) = size_zero_node;
  
  /* add information about the inheritance graph */
  if (base_type != NULL_TREE)
    {     
      copy_base_binfo (TYPE_BINFO (base_type), binfo);
    
      /* add field for base class members */
      field = scil_add_field (type, SCIL_ID_BASE_CLASS, base_type);
      SCIL_DECL_MANGLED (field) = get_identifier (SCIL_ID_BASE_CLASS_MANGLED);
    }
}

void
scil_make_vtable (tree type, unsigned int size)
{
  tree binfo, vtable, base_vtable, base_binfo;

  binfo = TYPE_BINFO (type);

  base_binfo = BINFO_N_BASE_BINFOS (binfo) > 0 ? 
               BINFO_BASE_BINFO (binfo, 0) : 
               NULL_TREE;

  if (base_binfo != NULL_TREE)
    {
      /* Copy vtable from the original binfo of the base type. */
      base_vtable = BINFO_VTABLE (TYPE_BINFO (TREE_TYPE (base_binfo)));
      BINFO_VTABLE (base_binfo) = base_vtable;
    }
  else
    {
      base_vtable = NULL_TREE;
    }
  
  vtable = scil_rt_make_vtable (type, size, base_vtable);
  DECL_CONTEXT (vtable) = type;
  DECL_ARTIFICIAL (vtable) = 1;
  TREE_STATIC (vtable) = 1;
  TREE_READONLY (vtable) = 1;
  DECL_VIRTUAL_P (vtable) = 1;
  /* Copied from cp/class.c:
     At one time the vtable info was grabbed 2 words at a time.  This
     fails on sparc unless you have 8-byte alignment.  (tiemann) */
  DECL_ALIGN (vtable) = MAX (TYPE_ALIGN (double_type_node), TARGET_VTABLE_ENTRY_ALIGN);
  TREE_PUBLIC (vtable) = 1;
  DECL_IGNORED_P (vtable) = 1;
  TREE_ADDRESSABLE (vtable) = 1;

  TYPE_VFIELD (type) = vtable;
  BINFO_VTABLE (binfo) = vtable;
  SET_DECL_ASSEMBLER_NAME (vtable, scil_mangle_vtable (type));
}

void
scil_set_vtable_slot (tree type, unsigned int index, tree method)
{
  tree vtable, addr;

  TREE_ADDRESSABLE (method) = 1;
  TREE_USED (method) = 1;
  DECL_PRESERVE_P (method) = 1;
  DECL_VIRTUAL_P (method) = 1;
  DECL_UNINLINABLE (method) = 1;
  DECL_INLINE (method) = 0;
  vtable = BINFO_VTABLE (TYPE_BINFO (type));
  addr = build1 (ADDR_EXPR, build_pointer_type (TREE_TYPE (method)), method);

  scil_rt_set_vtable_slot (vtable, index, addr);
  cgraph_node (method)->local.vtable_method = true;
  cgraph_mark_needed_node (cgraph_node (method));
}

unsigned int
scil_new_vtable_slot (tree type)
{
  return scil_rt_new_vtable_slot (BINFO_VTABLE (TYPE_BINFO (type)));
}

void
scil_complete_vtable (tree type)
{
  rest_of_decl_compilation (BINFO_VTABLE (TYPE_BINFO (type)), 0, 0);
}

tree
scil_add_field (tree type, const char *name, tree field_type)
{
  tree id, field;

  id = scil_get_nested_identifier (TYPE_NAME (type), name);

  field = scil_build_decl (FIELD_DECL, id, field_type);
  DECL_FIELD_CONTEXT (field) = type;
  SCIL_DECL_UNQUALIFIED (field) = get_identifier (name);

  /* The first base class that contains the field. Since we have the base class
     as a field we will never add base class fields to this class.
   */ 
  DECL_FCONTEXT (field) = type; 

  /* Add to fields of the class. The list of fields will be reversed when 
     completing the class, so we can add to front here.
  */
  TREE_CHAIN (field) = TYPE_FIELDS (type);
  TYPE_FIELDS (type) = field;

  /* Add to the TREE_LIST of declarations on that identifier. */
  SCIL_DECL (id) = tree_cons (NULL_TREE, field, SCIL_DECL (id));

  return field;
}

void
scil_complete_type (tree type)
{
  /* bring fields into right order */;
  TYPE_FIELDS (type) = nreverse (TYPE_FIELDS (type));

  layout_type (type);
  rest_of_type_compilation (type, 1);
}


tree 
scil_register_static_field (tree type, const char *name, tree field_type)
{
  tree id, field;
   
  id = scil_get_nested_identifier (TYPE_NAME (type), name);
  field = scil_build_decl (VAR_DECL, id, field_type);

  DECL_CONTEXT (field) = type;
  TREE_STATIC (field) = 1;
  TREE_PUBLIC (field) = 1;

  SCIL_DECL_UNQUALIFIED (field) = get_identifier (name);
  SET_DECL_ASSEMBLER_NAME (field, scil_mangle_field (field));
  
  /* Add to the TREE_LIST of declarations on that identifier. */
  SCIL_DECL (id) = tree_cons (NULL_TREE, field, SCIL_DECL (id));

  rest_of_decl_compilation (field, 0, 0);

  return field;
}


void
scil_add_memory_alias (tree field, HOST_WIDE_INT address)
{
  tree alias, type;

  type = TREE_TYPE (field);
  type = build_pointer_type (type);
  type = build_qualified_type (type, TYPE_QUAL_CONST);
  alias = build_int_cst (type, address);

  type = build_qualified_type (type, TYPE_QUAL_VOLATILE);
  alias = build1 (INDIRECT_REF, type, alias);

  TREE_SIDE_EFFECTS (alias) = 1;
  TREE_THIS_VOLATILE (alias) = 1;

  SCIL_MEMORY_ALIAS_P (alias) = 1;
  SCIL_DECL_MEMORY_ALIAS (field) = alias;
}
