/*
  CIL Compiler

  Almost main. Called by GCC's toplev.c

  Copyright (C) 2003, 2004  Jan Möller

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

/* cil-decl.h include: config.h system.h tree.h */
#include "cil-decl.h"
#include "toplev.h"
#include "opts.h"
#include "options.h"

#include "cil.h"
#include "parse_assembly.h"
#include "mangle.h"

const char **libdirs;
int num_libdirs;

static void add_libdir (const char *str);

static void cil_parse_assembly (void);

static const char *dirname (const char *path);

const char*
dirname (const char* path)
{
  size_t len = lbasename (path) - path;

  char *dir_name  = xmalloc (len + 1);
  memcpy (dir_name, path, len);
  dir_name[len] = '\0';

  return  dir_name;
}

void
add_libdir (const char *str)
{
  libdirs = (const char **) xrealloc (libdirs,
				      sizeof (char *) * (num_libdirs + 1));
  libdirs[num_libdirs] = str;
  ++num_libdirs;
}

/* Prepare to handle switches.  */
unsigned int
cil_init_options (unsigned int argc ATTRIBUTE_UNUSED,
		  const char **argv ATTRIBUTE_UNUSED)
{
  return CL_cil;
}

/* Language for usage for messages.  */

const char *const language_string = "CIL - Common Intermediate Language front end for GCC ";

/* Process a switch - called by opts.c. */
int
cil_handle_option (size_t scode, const char *arg, int value ATTRIBUTE_UNUSED)
{
  static int version_done = 0;

  enum opt_code code = (enum opt_code) scode;

  switch (code)
    {
    case OPT_version:
      if (!version_done) {
	fputs (language_string, stderr);
/*	fputs (version_string, stderr);*/
	fputs ("\n", stderr);
	version_done = 1;
      }
      break;
      			
    case OPT_L:
      add_libdir (arg);
      break;
    default:
      abort ();
    }
      
  return 1;
}

bool
cil_init (void)
{
  char const *dir;
  cil_init_decl_processing ();
  cil_init_mangle ();
  /* add directory of source file */
  if (in_fnames && in_fnames[0])
    {
      dir = dirname (lrealpath (in_fnames[0]));
      add_libdir (dir);
    }
  return true;
}


void
cil_parse_file (int debug_flag ATTRIBUTE_UNUSED)
{
  char *point;
  if (num_in_fnames != 1) 
    {
      fatal_error ("Expecting exactly one file");
      return;
    }
	
  if (!(point = strrchr (in_fnames[0], '.')) )
    {
      fatal_error ("Invalid input file: missing file extension");
      return;
    }

  if (!strcmp (point, ".exe") || !strcmp (point, ".dll") )
    {
      cil_parse_assembly ();
      return;
    }

  fatal_error ("Invalid input file: type not supported");
}

void
cil_parse_assembly (void)
{
  parse_assembly (in_fnames[0]);
}

void
cil_finish (void)
{
  free (libdirs);
}
