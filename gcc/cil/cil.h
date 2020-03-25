/*
  CIL Compiler

  Declarations for interfacing to cil1.c

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

#ifndef CIL_H
#define CIL_H

bool cil_init(void);

unsigned int cil_init_options (unsigned int argc ATTRIBUTE_UNUSED,
			       const char **argv ATTRIBUTE_UNUSED);
int cil_handle_option (size_t scode, const char *arg, int value);

void cil_parse_file (int debug_flag ATTRIBUTE_UNUSED);

void cil_finish (void);

#endif  /* CIL_H  */
