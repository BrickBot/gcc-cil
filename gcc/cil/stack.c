/*
  CIL Compiler

  Implememts a stack of trees.

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

#include "stack.h"
#include "ggc.h"
#include "errors.h"

static tree
convert_to_stack_type (tree node);

static tree
convert_item (tree type, tree node);

typedef struct _stack_item GTY(())
{
  tree GTY(()) node;
  struct _stack_item* GTY(()) previous;
} stack_item;

static GTY(()) stack_item *top_item = NULL;

static int height = 0;


tree
convert_to_stack_type (tree node)
{
  tree type;

  if (!node)
    {
      return node;
    }
	
  type = TREE_TYPE (node);
	
  switch (TREE_CODE (type))
    {
    case INTEGER_TYPE:
      {
	if (TYPE_PRECISION (type) < 32
	    && !CIL_TYPE_NATIVE (type))
	  {
	    node = convert (int32_type_node, node);
	  }
			
	if (TYPE_PRECISION (type) > 32)
	  {
	    node = convert (int64_type_node, node);
	  }
      }
    case BOOLEAN_TYPE:
    case POINTER_TYPE:
    case REAL_TYPE:
    case RECORD_TYPE: /* value type */
      {
	break;
      }
    default:
      {
	internal_error ("stack type not supported yet");
      }
    }

  return node;
}

static enum tree_code
integral_type (enum tree_code c)
{
  if (c == BOOLEAN_TYPE)return INTEGER_TYPE;
  return c;
}

tree
convert_item (tree type, tree node)
{
  if (type && node && (type != TREE_TYPE (node)))
    {
      enum tree_code from = integral_type (TREE_CODE (TREE_TYPE (node)));
      enum tree_code to = integral_type (TREE_CODE (type));
      if (type == boolean_type_node)
	node = convert (type, node);
      else if (from == INTEGER_TYPE && to == REAL_TYPE)
	node = build1 (FLOAT_EXPR, type, node);
      else if (from == REAL_TYPE && to == INTEGER_TYPE)
	node = build1 (FIX_TRUNC_EXPR, type, node);
      else if (from == INTEGER_TYPE && to == POINTER_TYPE)
	node = build1 (CONVERT_EXPR, type, node);
      else if (from == POINTER_TYPE && to == INTEGER_TYPE)
	node = build1 (CONVERT_EXPR, type, node);  
      else if (from == to)
	node = build1 (CONVERT_EXPR, type, node);
      else
	internal_error ("Cannot convert from %s to %s", 
			tree_code_name[from], tree_code_name[to]);
    }
	
  return node;
}

void
stack_convert_top_item (tree type)
{
  if (height > 0)
    {
      top_item->node = convert_item (type, top_item->node);
    }
}

void
stack_push (tree node)
{
  stack_item *new_item;

  new_item = (stack_item*) ggc_alloc (sizeof (stack_item));

  if (!new_item)
    {
      internal_error ("Out of memory");
    }
	
  node = convert_to_stack_type (node);

  new_item->node = node;
	
  new_item->previous = top_item;
	
  top_item = new_item;
	
  height++;
}

tree
stack_pop (tree type)
{
  tree node = NULL_TREE;

  if (height > 0)
    {
      node = top_item->node;
		
      top_item = top_item->previous;
  		
      height--;
		
      if (type)
	node = convert_item (type, node);
		
    }
	
  return node;
}

tree
stack_top (tree type)
{
  tree node = NULL_TREE;

  if (height > 0)
    {
      node = top_item->node;
		
      if (type)
	node = convert_item (type, node);
    }
	
  return node;
}

tree
stack_elem (int num, tree type)
{
  tree node = NULL_TREE;

  if (height > 0)
    {
      stack_item *item = top_item;

      int i;
      for (i = 1; i < num; ++i)
	{
	  item = item->previous;
	}

      node = item->node;

      node = convert_item (type, node);
    }
	
  return node;
}

void
stack_change_elem (int num, tree new_elem)
{
  if (height > 0)
    {
      stack_item *item = top_item;

      int i;
      for (i = 1; i < num; ++i)
	{
	  item = item->previous;
	}

      new_elem = convert_to_stack_type (new_elem);

      item->node = new_elem;
    }
}

int
stack_height (void)
{
  return height;
}

#include "gt-cil-stack.h"
