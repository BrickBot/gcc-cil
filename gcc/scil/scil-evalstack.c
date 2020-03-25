/* TODO: Add explanation on how the stack works. */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "ggc.h"
#include "toplev.h"

#include "scil-config.h"
#include "scil-callbacks.h"
#include "scil-builtins.h"
#include "scil-evalstack.h"


typedef struct stack_tag GTY(()) 
{
  /* always pointing above the top element, 
     never use directly but with stack_push or stack_pop */
  int top_index;

  /* The general (or mixed) evaluation stack.
     Can contain variables of any type.
  */
  tree stack;
} stack_t;

static GTY(()) stack_t stack;


typedef struct typed_stack_tag
{
  tree type;
  tree *stack;
} typed_stack_t;

/* Different stacks for different types for finding the
   right variable easily depending on type.
*/
static typed_stack_t address_stack;
static typed_stack_t float_stack;
static typed_stack_t int8_stack;
static typed_stack_t int16_stack;
static typed_stack_t int32_stack;
static typed_stack_t int64_stack;
static typed_stack_t valuetype_stack;

static tree*
make_stack (int size)
{
  int s = size * sizeof (tree);
  return (tree*) memset (xmalloc (s), 0, s);
}

void
scil_init_stacks (int size)
{
  stack.stack = NULL_TREE;
  stack.top_index = 0;

  int8_stack.type = scil_int8_type;
  int8_stack.stack = make_stack (size);
  int16_stack.type = scil_int16_type;
  int16_stack.stack = make_stack (size);
  int32_stack.type = scil_int32_type;
  int32_stack.stack = make_stack (size);
  int64_stack.type = scil_int64_type;
  int64_stack.stack = make_stack (size);
  float_stack.type = scil_float64_type;
  float_stack.stack = make_stack (size);
  address_stack.type = ptr_type_node;
  address_stack.stack = make_stack (size);
  valuetype_stack.type = NULL_TREE;
  valuetype_stack.stack = make_stack (size);
}

void
scil_free_stacks ()
{
  stack.stack = NULL_TREE;
  free (int8_stack.stack);
  free (int16_stack.stack);
  free (int32_stack.stack);
  free (int64_stack.stack);
  free (float_stack.stack);
  free (address_stack.stack);
  free (valuetype_stack.stack);
}

static typed_stack_t
get_stack (tree type)
{
  /* TODO: implement all types appropriately.
     ECMA 335 says we're allowed to track types with precision < 32 in more
     detail instead of mapping them to int32 when pushed on the stack.
  */

  if (type == scil_int32_type 
      || type == scil_uint32_type
     )
    return int32_stack;

  if (type == 0)
    internal_error ("No type specified for stack operation.");
  
  if (POINTER_TYPE_P (type))
    return address_stack;

  if (type == scil_int8_type
      || type == scil_bool_type
      || type == scil_uint8_type)
    return int8_stack;

  if (type == scil_int16_type
      || type == scil_char_type
      || type == scil_uint16_type)
    return int16_stack;

  if (type == scil_int64_type
      || type == scil_uint64_type)
    return int64_stack;
  
  if (type == scil_float32_type
      || type == scil_float64_type)
    return float_stack;

  /* Must be a value type then */  
  return valuetype_stack;
}

/* Returns the next variable that can be pushed on the stack */
tree
scil_get_stack_variable (tree type)
{
  tree var;
  typed_stack_t typed_stack;
  
  typed_stack = get_stack (type);
  var = typed_stack.stack [stack.top_index];

  if (typed_stack.stack != valuetype_stack.stack)
    {
      if (var == NULL_TREE)
        {
          typed_stack.stack [stack.top_index] = 
          var = scil_add_stack_variable (typed_stack.type);
        }
      return var;
    }
  else /* type is a valuetype */
    {
      tree v;
    
      /* search for the right variable */
      for (v = var; 
           v != NULL_TREE && TREE_PURPOSE (v) != type; 
           v = TREE_CHAIN (v));
      
      /* create new variable if necessary */
      if (v == NULL_TREE)
        {
          v = scil_add_stack_variable (type);
          typed_stack.stack [stack.top_index] = tree_cons (type, v, var);
          return v;
        }
      
      return TREE_VALUE (v);
    }
}

bool
scil_stack_is_empty ()
{
  return stack.top_index <= 0;
}

void
scil_clear_stack ()
{
  stack.stack = NULL_TREE;
  stack.top_index = 0;
}

#ifdef SCIL_TRACE_VIRTUAL_EXECUTION
static void
printout (const char *str, tree v)
{
  tree type = TREE_TYPE (v);
  if (POINTER_TYPE_P (type)) type = TREE_TYPE (type);
  tree name = TYPE_NAME (type);
  fnotice (stderr, "  %s %s %s\n", str, name ? IDENTIFIER_POINTER (name) : "<no name>", IDENTIFIER_POINTER (DECL_NAME (v)));
}
#else
#define printout(str, v)
#endif

void
scil_stack_push (tree stack_variable)
{
  stack.stack = tree_cons (NULL_TREE, stack_variable, stack.stack);
  printout ("push", stack_variable);
  stack.top_index++;
}

tree
scil_stack_pop ()
{
  --stack.top_index;
  tree r = TREE_VALUE (stack.stack);
  stack.stack = TREE_CHAIN (stack.stack);
  printout ("pop", r);
  return r;
}

tree
scil_stack_top ()
{
  return TREE_VALUE (stack.stack);
}

void *
scil_copy_stack ()
{
  stack_t *new_stack;

  new_stack = (stack_t *) ggc_alloc (sizeof (stack_t));
  new_stack->top_index = stack.top_index;
  new_stack->stack = copy_list (stack.stack);

  return (void *) new_stack;
}

void
scil_set_stack (void *s)
{
  stack_t *save;

  save = (stack_t *) s;
  
  stack.top_index = save->top_index;
  stack.stack = save->stack;
}

#include "gt-scil-scil-evalstack.h"
