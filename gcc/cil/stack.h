/*
	CIL Compiler

	Declarations for interfacing to stack.c

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
#ifndef CIL_STACK_H
#define CIL_STACK_H

/* cil-decl.h include: config.h system.h tree.h */
#include "cil-decl.h"

void stack_convert_top_item (tree type);

void stack_change_elem (int num, tree new_elem);

tree stack_elem (int num, tree type);

void stack_push (tree node);

tree stack_pop (tree type);

tree stack_top (tree type);

int stack_height (void);

#endif /* CIL_STACK_H  */

