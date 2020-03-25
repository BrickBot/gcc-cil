/* cil-decl.h include: config.h system.h tree.h */
#include "cil-decl.h"

/* GCC headers */
#include "toplev.h"

#include "pnet.h"
#include "parse_assembly.h"

static char*
find_assembly_path (const char *assem_name);

/* caller has to free the return value */
char*
find_assembly_path (const char *assem_name)
{
  char* path = 0;
  bool found = false;
  int i = 0;
        
  while (i < num_libdirs && !found)
    {
      /* create full path name */
      int liblen = strlen (libdirs[i]);
      path = alloca (liblen + strlen (assem_name) + 6);
      strcpy (path, libdirs[i]);
      strcat (path, "/");
      strcat (path, assem_name);
      strcat (path,".dll");

      found = (access (path, R_OK) >= 0);
      ++i;
    }
	
  if (found)
    {
      size_t len = strlen (path);
      char* ret = xmalloc (len + 1);
      strncpy (ret, path, len);
      ret[len] = '\0';
      return ret;
    }
  else
    {
      return 0;
    }
}

void
load_assembly (ILContext* context, ILImage** image, const char *filename)
{
  char *real_path = lrealpath (filename);
	
  int error = ILImageLoadFromFile (real_path,
				   context,
				   image,
				   IL_LOADFLAG_NO_RESOLVE,
				   1);
  if (error != 0)
    {
      const char* msg = ILImageLoadError (error);
      internal_error ("Error while loading assembly: %s", msg);
    }
  if (real_path )
    {
      free (real_path);
    }
}

void
load_mscorlib (ILContext* context)
{
  ILImage *systemImage;

  char *mscorlib_path = find_assembly_path ("mscorlib");

  if (mscorlib_path)
    {
      load_assembly (context, &systemImage, mscorlib_path);
      free (mscorlib_path);

      ILContextSetSystem (context, systemImage);
    }
  else
    {
      warning0 ("Cannot find mscorlib.dll");
    }
}

ILProgramItem*
get_class_ref_scope (ILClass *class_ref)
{
  ILProgramItem* scope = ILClassGetScope (class_ref);

  ILToken token = ILProgramItem_Token (scope) & IL_META_TOKEN_MASK;

  if (token != IL_META_TOKEN_ASSEMBLY_REF)
    {
      internal_error ("cannot find scope of external class '%s.%s'",
		      ILClassGetNamespace (class_ref),
		      ILClassGetName (class_ref));
    }
	
  return scope;
}

const char *
get_assembly_ref_name (ILClass *class_ref)
{
  ILProgramItem* scope = get_class_ref_scope (class_ref);

  return ILAssembly_Name (ILProgramItemToAssembly (scope));
}

ILClass*
resolve_class_ref (ILContext* context, ILClass* class_ref)
{
  ILImage* import_image;
  ILProgramItem* scope;
  ILClass* rclass;
  const char* assem_name;
        
  if (!ILClassIsRef (class_ref))
    {
      internal_error ("class '%s.%s' is not a reference",
		      ILClassGetNamespace (class_ref),
		      ILClassGetName (class_ref));
    }
	
  rclass = ILClassResolve (class_ref);

  if (rclass != class_ref)
    {
      return rclass;
    }
	
  assem_name = get_assembly_ref_name (class_ref);
	
  /* See if we already have an assembly with this name */
  import_image = ILContextGetAssembly (context, assem_name);
	
  if (import_image == 0)
    {
      /* load corresponding image */

      char* path = find_assembly_path (assem_name);

      if (path)
	{       /* load image */
	  load_assembly (context, &import_image, path);
	  free (path);
	}
      else
	{
	  fatal_error ("Cannot find %s.dll", assem_name);
	}
    }

  /* look up class within IMPORT_IMAGE */

  scope = ILClassGlobalScope (import_image);

  rclass = ILClassLookup (scope,
			  ILClassGetName (class_ref),
			  ILClassGetNamespace (class_ref));

  if (!rclass)
    {
      fatal_error ("Cannot find class '%s.%s' in '%s.dll'",
		   ILClassGetNamespace (class_ref),
		   ILClassGetName (class_ref),
		   assem_name);
    }

  if (!_ILProgramItemLink (&(class_ref->programItem),
			  &(rclass->programItem)))
    {
      internal_error ("Cannot resolve class '%s.%s'",
		      ILClassGetNamespace (class_ref),
		      ILClassGetName (class_ref));
    }
	
  return rclass;
}

