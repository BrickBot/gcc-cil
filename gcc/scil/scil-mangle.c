/* 
  SCIL front end name mangling according to
  http://www.codesourcery.com/cxx-abi/abi.html#mangling 
*/

/* TODO: explain concept
*/

#include <stdio.h>

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "obstack.h"
#include "toplev.h"

#include "scil-hooks.h"
#include "scil-callbacks.h"
#include "scil-rt.h"
#include "scil-trees.h"
#include "scil-mangle.h"

static struct obstack mangle_obstack;
static int compression_id;
static struct obstack compression_stack;

#define write(string, length) \
  obstack_grow (&mangle_obstack, string, length)

#define write1(c) \
  obstack_1grow (&mangle_obstack, c)

static void
compression_stack_push (tree node)
{
  if (!SCIL_NO_COMPRESSION (node))
    {
      obstack_ptr_grow (&compression_stack, node);
    }
}


static tree
get_context (tree node)
{
  return TYPE_P (node) ? TYPE_CONTEXT(node) : DECL_CONTEXT(node);
}

static void
set_mangled (tree node, tree mangled)
{
  if (TYPE_P (node))
    {
      SCIL_TYPE_MANGLED (node) = mangled;
    }
  else
    {
      SCIL_DECL_MANGLED (node) = mangled;
    }
}


void
scil_init_mangling (void)
{
  gcc_obstack_init (&mangle_obstack);
  gcc_obstack_init (&compression_stack);
  compression_id = -1;
}

static tree
complete_mangling (void)
{
  tree result;
  void *base;
  int length;

  /* harvest mangled name */
  length = obstack_object_size (&mangle_obstack);
  write1 ('\0');
  base = obstack_base (&mangle_obstack);
  result = get_identifier_with_length (base, length);
  obstack_free (&mangle_obstack, base);

  /* clean up compression stack */
  obstack_free (&compression_stack, obstack_base (&compression_stack));

  return result;
}


/* The lowest valid ID is -1. Thus, it returns something smaller than that for
   items that are not already marked as used.
*/
static int
get_compression_id (tree node)
{
  int id;
  tree *t, *end;

  id = -1;
  end = (tree *) obstack_next_free (&compression_stack);
  for (t = (tree *) obstack_base (&compression_stack); t < end; ++t, ++id)
    {
      if (*t == node)
        return id;
    }
  return -2;
}

static void
write_identifier (tree id)
{
  write (IDENTIFIER_POINTER (id), IDENTIFIER_LENGTH (id));
}


/* writes out the compression string for the given id */
static void
write_compression (int id)
{
  write1 ('S');
  if (id >= 0)
    {
      static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      unsigned HOST_WIDE_INT n;
      unsigned HOST_WIDE_INT power = 1;
      for (n = id; n >= 36; n /= 36)
         power *= 36;
      /* Write the digits out */
      while (power > 0)
        {
          int digit = id / power;
          write1 (digits [digit]);
          id -= digit * power;
          power /= 36;
        }
    }
  write1 ('_');
}


static tree
make_chunk (tree node)
{
  /* TODO: for mangling of generics, add cases here */

  tree name;
  char *buf;
  int length;

  name = SCIL_UNQUALIFIED (node);    
  length = IDENTIFIER_LENGTH (name);
  buf = alloca (length + 11);
  length = sprintf (buf, "%d%s", length, IDENTIFIER_POINTER (name));
  
  name = get_identifier_with_length (buf, length);
  set_mangled (node, name);
  return name;
}

static void
write_chunk (tree node)
{
  tree chunk;

  chunk = SCIL_MANGLED (node);
  if (!chunk)
    {
      chunk = make_chunk (node);
    }

  write_identifier (chunk);
}


/* returns true if NODE was already on the compression stack */
static void
write_chunk_or_compression (tree node, bool brackets)
{
  int id;
  tree container;

  /* look for compression and write it */
  if (!SCIL_NO_COMPRESSION (node))
    {
      id = get_compression_id (node);
      if (id >= -1)
        {
          write_compression (id);
          return;
        }
    }

  /* look for container and write it */
  container = get_context (node);
  if (container)
    {
      if (brackets) write1 ('N');
      write_chunk_or_compression (container, false);
    }
  write_chunk (node);
  if (brackets && container) write1 ('E');
  
  compression_stack_push (node);
}

static void
write_type (tree type)
{
  if (TREE_CODE (type) == POINTER_TYPE)
    {
      write1 ('P');
      write_type (TREE_TYPE (type));
    }
  else 
    {
      write_chunk_or_compression (type, true);
    }
}

static void
write_method_type (tree type, bool static_p)
{
  tree element, arg_type;

  element = TYPE_ARG_TYPES (type);

  /* omit implicit instance argument */
  if (!static_p)
    element = TREE_CHAIN (element);

  arg_type = TREE_VALUE (element);
  if (arg_type == void_type_node)
    {
      write1 ('v');
    }
  else
    {
      do
        {
          write_type (arg_type);        
          element = TREE_CHAIN (element);
          arg_type = TREE_VALUE (element);
        }
      while (element != NULL_TREE && arg_type != void_type_node);
    }
  
}

static void
write_method (tree decl)
{
  tree type;

  /* special cases for constructors */
  if (!strcmp(".ctor", IDENTIFIER_POINTER (SCIL_UNQUALIFIED (decl))))
    {
      set_mangled (decl, get_identifier ("C1"));
    }
  else if (!strcmp(".cctor", IDENTIFIER_POINTER (SCIL_UNQUALIFIED (decl))))
    {
      set_mangled (decl, get_identifier ("6cctor$"));
    }
  write_chunk_or_compression (decl, true);
  
  type = TREE_TYPE (decl);
  write_method_type (type, TREE_CODE (type) == FUNCTION_TYPE);
}

tree
scil_mangle_method (tree decl)
{
  /* write method name */
  write ("_Z", 2);
 
  write_method (decl);
 
  return complete_mangling ();
}

tree
scil_mangle_field (tree decl)
{
  /* write name */
  write ("_Z", 2);
  write_chunk_or_compression (decl, true);

  return complete_mangling ();
}

tree
scil_mangle_vtable (tree type)
{
  /* write name */
  write ("_ZTV", 4);
  write_chunk_or_compression (type, true);

  return complete_mangling ();
}

tree
scil_get_label_id (tree method, const char *name)
{
  char buf[11];
  int length;

  /* Z starts a local name */
  write ("_ZZ", 3);
  write_method (method);

  /* write label name */
  length = strlen (name);
  write (buf, sprintf (buf, "%d", length));
  write (name, length);

  /* E ends a scope */
  write1 ('E');

  return complete_mangling ();
}
