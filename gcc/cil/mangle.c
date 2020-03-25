/*
  CIL Compiler

  Implement C++ compatible name mangling

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

  Written by Martin v. Löwis 2005, based in part on other
  parts of the GCC compiler.
*/

#include "cil-decl.h"

#include "toplev.h"
#include "obstack.h"
#include "pnet.h"
#include "mangle.h"

/* Based on java/mangle.c */

/* The compression table is a list that keeps track of things we've
   already seen, so they can be reused. For example,
   [mscorlib]System.Object would generate three entries: an assembly
   name, a namespace, name and a type. If [mscorlib]System.String is
   presented next, the [mscorlib]System will be matched against the
   first two entries (and kept for compression as S0_), and type
   String would be added to the list. */

enum compression_type{ CLASS, POINTER, ARRAY, NAMESPACE, ASSEMBLY, SPECIAL };

typedef struct centry {
  int value;
  enum compression_type type;
  union{
    ILClass *klass;
    ILType *pointer; /* or array */
    const char *assembly;
    struct {
      struct centry *container;
      char *namespace;
    } ns;
  }u;
  struct centry *prev;
} centry;

struct centry* ctable;
  
struct obstack compression_obstack;

struct obstack mangle_obstack;

static void init_mangling (void);
static tree finish_mangling (void);
static void mangle_type (ILType *);
static centry* alloc_entry (void);

/* Mangling initialization routine.  */

static void init_mangling (void)
{
  if (ctable)
    /* Mangling already in progress.  */
    abort ();

  /* Mangled name are to be suffixed */
  obstack_grow (&mangle_obstack, "_Z", 2);
}

/* Mangling finalization routine. The mangled name is returned as a
   IDENTIFIER_NODE.  */

static tree
finish_mangling (void)
{
  tree result;

  ctable = NULL;
  obstack_1grow (&mangle_obstack, '\0');
  result = get_identifier (obstack_base (&mangle_obstack));
  obstack_free (&mangle_obstack, obstack_base (&mangle_obstack));
  obstack_free (&compression_obstack, obstack_base (&compression_obstack));
  return result;
}

static centry*
alloc_entry (void)
{
  centry *result = (centry*)obstack_alloc (&compression_obstack, 
					   sizeof (centry));

  result->prev = ctable;
  if (ctable)
    result->value = ctable->value + 1;
  else
    result->value = 0;
  ctable = result;
  return result;
}      

/* Write a substitution string for entry I. Substitution string starts a
   -1 (encoded S_.) The base is 36, and the code shamelessly taken from
   cp/mangle.c.  */

static void
emit_compression_string (int i)
{
  i -= 1;                       /* Adjust */
  obstack_1grow (&mangle_obstack, 'S');
  if (i >= 0)
    {
      static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      unsigned HOST_WIDE_INT n;
      unsigned HOST_WIDE_INT m=1;
      /* How many digits for I in base 36? */
      for (n = i; n >= 36; n /= 36, m *=36);
      /* Write the digits out */
      while (m > 0)
        {
          int digit = i / m;
          obstack_1grow (&mangle_obstack, digits [digit]);
          i -= digit * m;
          m /= 36;
        }
    }
  obstack_1grow (&mangle_obstack, '_');
}

static void
mangle_name (const char *name)
{
  char buf[10];
  int len = strlen (name);
  int buflen;

  buflen = sprintf (buf, "%d", len);
  obstack_grow (&mangle_obstack, buf, buflen);
  obstack_grow (&mangle_obstack, name, len);
}

static centry*
mangle_assembly_or_special (const char *name, enum compression_type t)
{
  centry *prev;

  for (prev = ctable; prev; prev = prev->prev)
    {
      if (prev->type == t
	  && strcmp (name, prev->u.assembly) == 0)
	{
	  emit_compression_string (prev->value);
	  return prev;
	}
    }

  prev = alloc_entry ();
  prev->type = t;
  prev->u.assembly = name;

  mangle_name (name);
  return prev;
}

static centry*
mangle_assembly (const char* name)
{
  return mangle_assembly_or_special (name, ASSEMBLY);
}

static void
mangle_special (const char* name)
{
  mangle_assembly_or_special (name, SPECIAL);
}

static int
entry_matches (centry *e, const char* assembly, const char *namespace, int levels)
{
  int i;
  const char *name;
  if (levels == 0)
    return (e->type == ASSEMBLY) && (strcmp (e->u.assembly, assembly) == 0);
  if (e->type != NAMESPACE)
    return 0;
  if (!entry_matches (e->u.ns.container, assembly, namespace, levels-1))
    return 0;
  for (i = 0, name = namespace; i < levels; i++)
    {
      while (*name != '.')
        name++;
      name++;
    }
  return strncmp (e->u.assembly, name, strlen (e->u.assembly)) == 0;
}

static centry *
mangle_namespace_r (const char *assembly, const char *namespace, int levels)
{
  centry *prev, *result;
  const char *name, *iter;

  for (prev = ctable; prev; prev = prev->prev)
    {
      if (entry_matches (prev, assembly, namespace, levels))
	{
	  emit_compression_string (prev->value);
	  return prev;
	}
    }

  if (levels == 0)
    return mangle_assembly (assembly);

  prev = mangle_namespace_r (assembly, namespace, levels - 1);

  for (name = namespace; levels>1; levels--)
    {
      while (*name != '.')
        name++;
      name++;
    }
  for (iter = name; *iter != '\0' && *iter != '.'; iter++);

  result = alloc_entry ();
  result->type = NAMESPACE;
  result->u.ns.container = prev;
  result->u.ns.namespace = (char*)obstack_alloc (&compression_obstack, iter-name+1);
  strncpy (result->u.ns.namespace, name, iter-name);
  result->u.ns.namespace[iter-name] = '\0';

  mangle_name (result->u.ns.namespace);
  return prev;
}

static void
mangle_namespace (const char* assembly, const char* namespace)
{
  int levels = 1;
  const char* iter;
  for (iter = namespace; *iter; iter++)
    if (*iter == '.')
      levels++;
  mangle_namespace_r (assembly, namespace, levels);
}

static const char*
assembly_name (ILClass* klass)
{
  if (!ILClassIsRef (klass))
    return ILImageGetAssemblyName (ILClassToImage (klass));
  else
    return get_assembly_ref_name (klass);
}

static int
equal_class (ILClass *k1, ILClass *k2)
{
  if (k1 == k2)
    return 1;
  if (strcmp (ILClass_Name (k1), ILClass_Name (k2)) != 0)
    return 0;
  if (strcmp (ILClass_Namespace (k1), ILClass_Namespace (k2)) != 0)
    return 0;
  if (strcmp (assembly_name (k1), assembly_name (k2)) != 0)
    return 0;
  return 1;
}

static void 
mangle_class (ILClass *klass)
{
  centry *prev;
  const char *assembly;

  for (prev = ctable; prev; prev = prev->prev)
    {
      if (prev->type == CLASS
	  && equal_class (prev->u.klass, klass))
	{
	  emit_compression_string (prev->value);
	  return;
	}
    }

  if (ILClass_IsNestedPublic (klass)
      || ILClass_IsNestedPrivate (klass)
      || ILClass_IsNestedFamily (klass))
    {
      mangle_class (ILClass_NestedParent (klass));
    }
  else
    {
      assembly = assembly_name (klass);
      
      if (ILClass_Namespace (klass))
	mangle_namespace (assembly, ILClass_Namespace (klass));
      else
	mangle_assembly (assembly);
    }

  mangle_name (ILClass_Name (klass));

  prev = alloc_entry ();
  prev->type = CLASS;
  prev->u.klass = klass;
}

static void
mangle_pointer (ILType *il_type)
{
  centry *prev;

  for (prev = ctable; prev; prev = prev->prev)
    {
      if (prev->type == POINTER
	  && prev->u.pointer == il_type)
	{
	  emit_compression_string (prev->value);
	  return;
	}
    }

  if (ILType_IsClass (il_type))
    {
      obstack_grow (&mangle_obstack, "PN", 2);
      mangle_class (ILType_ToClass (il_type));
      obstack_1grow (&mangle_obstack, 'E');
    }
  else if (ILType_IsValueType (il_type))
    {
      obstack_grow(&mangle_obstack, "N", 1);
      mangle_class (ILType_ToClass (il_type));
      obstack_1grow (&mangle_obstack, 'E');
    }
  else
    {
      obstack_1grow (&mangle_obstack, 'P');
      mangle_type (ILType_Ref (il_type));
    }
  
  prev = alloc_entry ();
  prev->type = POINTER;
  prev->u.pointer = il_type;
}

static void
mangle_array (ILType *il_type)
{
  centry *prev;

  for (prev = ctable; prev; prev = prev->prev)
    {
      if (prev->type == ARRAY
	  && prev->u.pointer == il_type)
	{
	  emit_compression_string (prev->value);
	  return;
	}
    }

  /* ILArray<type>* */
  obstack_1grow (&mangle_obstack, 'P');
  mangle_special ("ILArray");
  obstack_1grow (&mangle_obstack, 'I');
  mangle_type (ILTypeGetElemType (il_type));
  obstack_1grow (&mangle_obstack, 'E');

  prev = alloc_entry ();
  prev->type = ARRAY;
  prev->u.pointer = il_type;
}
      
static void
mangle_value (ILType *il_type)
{
  centry *prev;
  ILClass *klass = ILType_ToClass (il_type);

  for (prev = ctable; prev; prev = prev->prev)
    {
      if (prev->type == CLASS
	  && prev->u.klass == klass)
	{
	  emit_compression_string (prev->value);
	  return;
	}
    }

  obstack_1grow (&mangle_obstack, 'N');
  mangle_class (klass);
  obstack_1grow (&mangle_obstack, 'E');
}
 

static void
mangle_type (ILType *il_type)
{
  if (ILType_IsPrimitive (il_type))
    {
      switch (ILType_ToElement (il_type))
	{
	case IL_META_ELEMTYPE_VOID:
	  obstack_1grow (&mangle_obstack, 'v');
	  break;
	case IL_META_ELEMTYPE_BOOLEAN:
	  obstack_1grow (&mangle_obstack, 'b');
	  break;
	case IL_META_ELEMTYPE_CHAR:				  
	  obstack_1grow (&mangle_obstack, 'w');
	  break;
	case IL_META_ELEMTYPE_I1:
	  obstack_1grow (&mangle_obstack, 'a');
	  break;
	case IL_META_ELEMTYPE_U1:
	  obstack_1grow (&mangle_obstack, 'h');
	  break;
	case IL_META_ELEMTYPE_I2:
	  obstack_1grow (&mangle_obstack, 's');
	  break;
	case IL_META_ELEMTYPE_U2:
	  obstack_1grow (&mangle_obstack, 't');
	  break;
	case IL_META_ELEMTYPE_I4: /* TODO: verify that long is 32 bit */
	  obstack_1grow (&mangle_obstack, 'l');
	  break;
	case IL_META_ELEMTYPE_U4:
	  obstack_1grow (&mangle_obstack, 'm');
	  break;
	case IL_META_ELEMTYPE_I8:
	  obstack_1grow (&mangle_obstack, 'x');
	  break;
	case IL_META_ELEMTYPE_U8:
	  obstack_1grow (&mangle_obstack, 'y');
	  break;
	case IL_META_ELEMTYPE_R4:
	  obstack_1grow (&mangle_obstack, 'f');
	  break;
	case IL_META_ELEMTYPE_R8:
	  obstack_1grow (&mangle_obstack, 'd');
	  break;
	case IL_META_ELEMTYPE_TYPEDBYREF:
	  internal_error ("Type not implemented");
	  break;
	case IL_META_ELEMTYPE_I:
	  obstack_1grow (&mangle_obstack, 'i');
	  break;
	case IL_META_ELEMTYPE_U:
	  obstack_1grow (&mangle_obstack, 'j');
	  break;
	case IL_META_ELEMTYPE_R:
	  internal_error ("Type not implemented");
	  break;
	case IL_META_ELEMTYPE_SENTINEL:
	default:
	  internal_error ("Type not implemented");
	}
    }
  else if (ILType_IsClass (il_type))
    {
      mangle_pointer (il_type);
    }
  else if (ILType_Kind (il_type) == IL_TYPE_COMPLEX_BYREF)
    {
      mangle_pointer (il_type);
    }
  else if (ILType_IsValueType (il_type))
    {
      mangle_value (il_type);
    }
  else if (ILType_IsSimpleArray (il_type))
    {
      mangle_array (il_type);
    }
  else if (ILType_IsArray (il_type))
    {
      internal_error ("Type not implemented");
    }
  else
    internal_error ("Type not implemented");
}
  
tree 
mangle_function (ILMethod *method)
{
  ILType *sig;
  int param, num_params;

  init_mangling ();

  obstack_1grow (&mangle_obstack, 'N');

  mangle_class (ILMethod_Owner (method));

  if (strcmp (ILMethod_Name (method), ".ctor") == 0)
    {
      /* Constructor, always in charge */
      obstack_grow (&mangle_obstack, "C1", 2);
    }
  else if (strcmp (ILMethod_Name (method), ".cctor") == 0)
    {
      /* Class ctor, no predefined mangling */
      obstack_grow (&mangle_obstack, "6cctor$", 7);
    }
  else
    {
      mangle_name (ILMethod_Name (method));
    }

  obstack_1grow (&mangle_obstack, 'E');

  sig = ILMethod_Signature (method);

  num_params = ILTypeNumParams (sig);
  
  for (param = 1; param <= num_params; ++param)
    {
      ILType *il_type = ILTypeGetParam (sig, param);
      mangle_type (il_type);
    }

  if (num_params == 0)
    {
      /* Function with no arguments, mangle () */
      obstack_1grow (&mangle_obstack, 'v');
    }

  return finish_mangling ();
}

tree
mangle_mono_class (ILClass *klass)
{
  init_mangling ();
  mangle_class (klass);
  obstack_grow (&mangle_obstack, "$mono_class", 11);
  return finish_mangling ();
}

tree
mangle_mono_class_runtime_info (ILClass *klass)
{
  init_mangling ();
  mangle_class (klass);
  obstack_grow (&mangle_obstack, "$mono_class_runtime_info", 24);
  return finish_mangling ();
}

tree
mangle_vtable (ILClass *klass)
{
  init_mangling ();
  obstack_grow (&mangle_obstack, "TVN", 3);
  mangle_class (klass);
  obstack_1grow (&mangle_obstack, 'E');
  return finish_mangling ();
}

tree
mangle_class_data (ILClass *klass)
{
  init_mangling ();
  mangle_class (klass);
  obstack_grow (&mangle_obstack, "$class_data", 11);
  return finish_mangling ();
}

void
cil_init_mangle (void)
{
  gcc_obstack_init (&mangle_obstack);
  gcc_obstack_init (&compression_obstack);
}
