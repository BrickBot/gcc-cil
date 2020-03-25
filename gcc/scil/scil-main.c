#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "options.h"
#include "toplev.h"
#include "cgraph.h"
#include "tree-inline.h"

#include "ggc.h"
#include "obstack.h"

#include "scil-hooks.h"
#include "scil-cral.h"
#include "scil-builtins.h"
#include "scil-mangle.h"

/* Global variables, as declared by scil_hook_pushdecl. */
static tree globals;

/* The directories that are to be searched for assemblies. */
static struct obstack directories;


/* see lang_hooks.init_options in langhooks.h */
unsigned int
scil_hook_init_options (unsigned int argc ATTRIBUTE_UNUSED,
                        const char **argv ATTRIBUTE_UNUSED)
{
  obstack_init (&directories);

  return CL_SCIL;
}


/* see lang_hooks.handle_option in langhooks.h */
int scil_hook_handle_option (size_t code, 
                             const char *arg,
                             int value ATTRIBUTE_UNUSED)
{
  switch (code)
  {
    case OPT_I:
      obstack_ptr_grow (&directories, arg);
      return 1;
  
    case OPT_syslib:
      return -1;
  }
  return 0;
}

bool
scil_hook_post_options (const char **pfilename ATTRIBUTE_UNUSED)
{
  /* Use tree inlining.  */
  flag_inline_trees = 1;

  if (!flag_no_inline)
    {
      flag_no_inline = 1;
    }
  
  if (flag_inline_functions)
    {
      flag_inline_trees = 2;
    }
  
  return false;
}


/* see lang_hooks.init in langhooks.h */
bool 
scil_hook_init ()
{
  int dirs;

  globals = NULL_TREE;

  scil_init_mangling ();
  scil_make_builtins ();
  
  if (!cral_init ()) 
    return false;

  dirs = obstack_object_size (&directories);
  cral_set_search_directories (dirs, (char **) obstack_finish (&directories));
  
  cral_load_system_library (scil_system_library);
  
  return true;
}


/* see lang_hooks.parse_file in langhooks.h */
void
scil_hook_parse_file (int unused ATTRIBUTE_UNUSED)
{
  cral_parse_file (main_input_filename);  
  
  current_function_decl = SCIL_DECL (main_identifier_node);
  cgraph_node (current_function_decl);
  cgraph_finalize_function(current_function_decl, false);

  cgraph_finalize_compilation_unit ();
  cgraph_optimize ();
}


/* see lang_hooks_for_decls.final_write_globals in langhooks.h */
void
scil_hook_final_write_globals (void)
{
  tree decl;

  for (decl = globals; decl != NULL_TREE; decl = TREE_CHAIN (decl))
    {
      rest_of_decl_compilation (decl, 1, 1);
    }
}

/* see lang_hooks_for_decls.pushdecl in langhooks.h */
/* We only have the global level. */
tree
scil_hook_pushdecl (tree decl)
{
  TREE_CHAIN (decl) = globals;
  globals = decl;
  return decl;
}


/* see lang_hooks.finish in langhooks.h */
void
scil_hook_finish (void)
{
  cral_finalize ();
  obstack_free (&directories, 0);
}
