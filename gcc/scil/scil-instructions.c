#include <stdio.h>

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "toplev.h"
#include "tree-gimple.h"

#include "scil-config.h"
#include "scil-hooks.h"
#include "scil-callbacks.h"
#include "scil-rt.h"
#include "scil-trees.h"
#include "scil-builtins.h"
#include "scil-mangle.h"
#include "scil-instructions.h"
#include "scil-evalstack.h"

/* Functions in this file are not stateless, ie you can compile only one
   method at a time. */

static tree get_label (unsigned int);
static tree instr_push (tree, tree);
static tree instr_pop (tree);
static tree instr_resolve_field (tree);
static tree instr_pop_parameters (tree);
static tree instr_unop (enum tree_code);
static tree instr_binop (enum tree_code);
static tree instr_binop_typed (enum tree_code, tree);
static tree instr_binop_un (enum tree_code);
static tree instr_binop_un_typed (enum tree_code, tree);
static tree instr_goto (unsigned int target);
static void append_unop (enum tree_code);
static void append_binop (enum tree_code);
static void append_binop_un (enum tree_code);
static void append_compare (enum tree_code);
static void append_compare_un (enum tree_code, enum tree_code);
static void append_binop_cases (enum tree_code, enum tree_code);
static tree pop_convert (tree);
static tree use_alias (tree);

tree
get_label (unsigned int program_counter)
{
  char name[16];
  tree label_id, label;

  sprintf (name, SCIL_ID_LABEL, program_counter);
  label_id = scil_get_label_id (current_function_decl, name);
  label = SCIL_DECL (label_id);
  
  if (!label) 
    {
      label = scil_build_decl (LABEL_DECL, label_id, void_type_node);
      SCIL_DECL (label_id) = label;
      DECL_CONTEXT (label) = current_function_decl;
      DECL_MODE (label) = VOIDmode;
      FORCED_LABEL (label) = 0;
    }
  
  return label;  
}



void
scil_set_label (unsigned int program_counter)
{
  tree label, expr;
  
  label = get_label (program_counter);
  
  /* If we are target of a forward branch we must set the stack here as it was
     when the branch occurred. */
  if (SCIL_DECL_DATA (label))
    {
      /* If not set by the code below */
      if (SCIL_DECL_DATA (label) != (void *) 1)
        {
          scil_set_stack (SCIL_DECL_DATA (label));
        }
    }
  else
    {
      /* Make sure we don't save the stack later.
         We can be quite sure that 1 is never a valid pointer. */
      SCIL_DECL_DATA (label) = (void *) 1;
    }

  expr = build1 (LABEL_EXPR, void_type_node, label);
  scil_append_stmt (expr);
}


tree 
pop_convert (tree type)
{
  return build1 (CONVERT_EXPR, type, scil_stack_pop ());
}

tree 
use_alias_if_any (tree expr)
{
  if (DECL_P (expr) && SCIL_DECL_MEMORY_ALIAS (expr) != NULL_TREE)
    {
      return SCIL_DECL_MEMORY_ALIAS (expr);
    }
  
  return expr;
}




/* Helper for all the instructions that push on the stack.
   Returns an expression for pushing value of type TYPE from the variable 
   denoted by EXPR on the stack. */
tree
instr_push (tree type, tree expr)
{
  tree stack_type, stack_var, convert_expr;

  expr = use_alias_if_any (expr);

  type = TYPE_MAIN_VARIANT (type);
  stack_var = scil_get_stack_variable (type);
  
  stack_type = TREE_TYPE (stack_var);
  convert_expr = build1 (CONVERT_EXPR, stack_type, expr);
  
  scil_stack_push (stack_var);
  
  return build2 (MODIFY_EXPR, stack_type, stack_var, convert_expr);
}


/* Helper for functions that pop from the stack.
   Returns the expression for popping top element into the variable denoted by VAR. */
tree
instr_pop (tree var)
{
  tree type;

}


/* Returns an expression for the FIELD of the object which is on the stack. */
tree 
instr_resolve_field (tree field)
{
  tree type, object;

  type = TREE_TYPE (field);
  object = scil_stack_pop ();

  /* If we have a memory alias on the stack we use this field as a memory alias */
  if (SCIL_DECL_MEMORY_ALIAS (field) != NULL_TREE)
    {
      tree expr, addr, ptr_type;

      field = SCIL_DECL_MEMORY_ALIAS (field);
      ptr_type = TREE_TYPE (field);
    
      addr = build1 (ADDR_EXPR, ptr_type, field);
      expr = build1 (NOP_EXPR, ptr_type, object);
      expr = build2 (PLUS_EXPR, ptr_type, object, addr);
      expr = build1 (INDIRECT_REF, type, expr);

      TREE_SIDE_EFFECTS (expr) = 1;
      TREE_THIS_VOLATILE (expr) = 1;
    
      return expr;
    }
  
  if (POINTER_TYPE_P (TREE_TYPE (object)))
    {
      tree t;
    
      t = DECL_CONTEXT (field);
      object = build1 (NOP_EXPR, build_pointer_type (t), object);   
      object = build1 (INDIRECT_REF, t, object);
    }

  return build3 (COMPONENT_REF, type, object, field, NULL_TREE);
}

/* Pops arguments from the stack, converting them to the types given in the 
   list PARAMETER_TYPES and returns a list of those parameters.
*/
tree
instr_pop_parameters (tree parameter_types)
{
  tree parameters, p;

  /* TODO: support varargs */

  /* build PARAMETERS chain, but with types in reverse order because the first
     parameter is last on the stack.
  */
  parameters = NULL_TREE;
  while (parameter_types != NULL_TREE)
    {
      parameters = tree_cons (TREE_VALUE (parameter_types), 
                              NULL_TREE, parameters);
      parameter_types = TREE_CHAIN (parameter_types);
    }

  /* iterate PARAMETERS chain, build its reverse in P, pop the arguments in the
     TREE_VALUEs and remove the types from TREE_PURPOSE
  */
  p = NULL_TREE;
  while (parameters != NULL_TREE)
    {
      tree ptype = TREE_PURPOSE (parameters);
    
      if (ptype != void_type_node)
        {
          tree t = p;
        
          p = parameters;
          parameters = TREE_CHAIN (parameters);
          TREE_CHAIN (p) = t;
         
          TREE_VALUE (p) = pop_convert (ptype);
          TREE_PURPOSE (p) = NULL_TREE;
        }
      else 
        {
          parameters = TREE_CHAIN (parameters);
        }
    }

  return p;
}



/* Helper for binary operations: takes 2 arguments from the stack and pushes
   the result of OPERATION on the stack.
   Returns the respective expression tree.
*/

/* makes a unary operation instruction */
tree
instr_unop (enum tree_code operation)
{
  tree op = scil_stack_pop ();
  tree type = TREE_TYPE (op);
  tree expr = build1 (operation, type, op);

  return instr_push (type, expr);
}

/* makes a binary operation instruction for signed integers or reals */
tree
instr_binop (enum tree_code operation)
{
  tree op1, op2, type1, type2, expr;

  op2 = scil_stack_pop ();
  op1 = scil_stack_pop ();
  type1 = TREE_TYPE (op1);
  type2 = TREE_TYPE (op2);

  /* use the bigger type as the type for the operation */
  if (TYPE_PRECISION (type1) < TYPE_PRECISION (type2))
    {
      type1 = type2;
      op1 = build1 (CONVERT_EXPR, type2, op1);
    }
  else if (TYPE_PRECISION (type1) > TYPE_PRECISION (type2))
    {
      op2 = build1 (CONVERT_EXPR, type1, op2);
    }
  
  expr = build2 (operation, type1, op1, op2);

  return instr_push (type1, expr);
}

/* as instr_binop, but type is given explicitly and not derived from stack values */
tree
instr_binop_typed (enum tree_code operation, tree type)
{
  tree op1, op2, type1, type2, expr;

  op2 = scil_stack_pop ();
  op1 = scil_stack_pop ();
  type1 = TREE_TYPE (op1);
  type2 = TREE_TYPE (op2);

  /* use the bigger type as the type for the operation */
  if (TYPE_PRECISION (type1) < TYPE_PRECISION (type2))
    {
      op1 = build1 (CONVERT_EXPR, type2, op1);
    }
  else if (TYPE_PRECISION (type1) > TYPE_PRECISION (type2))
    {
      op2 = build1 (CONVERT_EXPR, type1, op2);
    }

  expr = build2 (operation, type, op1, op2);

  return instr_push (type, expr);
}

/* makes a binary operation instruction for unsigned integers */
tree
instr_binop_un (enum tree_code operation)
{
  tree s = scil_stack_top ();
  tree type = scil_hook_unsigned_type (TREE_TYPE (s));

  tree op2 = pop_convert (type);
  tree op1 = pop_convert (type);
  tree expr = build2 (operation, type, op1, op2);

  return instr_push (type, expr);
}

/* makes a binary operation instruction for unsigned integers */
tree
instr_binop_un_typed (enum tree_code operation, tree rtype)
{
  tree s = scil_stack_top ();
  tree type = scil_hook_unsigned_type (TREE_TYPE (s));

  tree op2 = pop_convert (type);
  tree op1 = pop_convert (type);
  tree expr = build2 (operation, rtype, op1, op2);

  return instr_push (rtype, expr);
}

/* appends a unary operation instruction */
void
append_unop (enum tree_code code)
{
  scil_append_stmt (instr_unop (code));
}

/* appends a binary operation instruction for signed integers or reals */
void
append_binop (enum tree_code code)
{
  scil_append_stmt (instr_binop (code));
}

/* appends a binary operation instruction for unsigned integers */
void
append_binop_un (enum tree_code code)
{
  scil_append_stmt (instr_binop_un (code));
}

void
append_compare (enum tree_code code)
{
  scil_append_stmt (instr_binop_typed (code, scil_bool_type));
}


/* appends a compare instruction for unsigned integers or reals with check for
   unordered-ness */
void
append_compare_un (enum tree_code int_code, enum tree_code real_code)
{
  tree stmt;
  tree type = TREE_TYPE (scil_stack_top ());
  if (TREE_CODE (type) == INTEGER_TYPE)
    {
      stmt = instr_binop_un_typed (int_code, scil_bool_type);
    }
  else
    {
      stmt = instr_binop_typed (real_code, scil_bool_type);
    }
  scil_append_stmt (stmt);
}

/* appends a binary operation instruction that has different GCC codes for
   signed integers and reals */
void
append_binop_cases (enum tree_code int_code, enum tree_code real_code)
{
  tree type = TREE_TYPE (scil_stack_top ());
  enum tree_code code = TREE_CODE (type) == INTEGER_TYPE ? int_code : real_code;

  scil_append_stmt (instr_binop (code));
}

tree
instr_goto (unsigned int target)
{
  tree label; 

  label = get_label (target);
  FORCED_LABEL (label) = 1;

  /* If we didn't save the stack for target save it now.
     We need not save it twice (and possibly merge the stacks) because ECMA 335
     explicitly says that stacks must be the same regarding height and types of
     its elements for all paths.
  */
  if (!SCIL_DECL_DATA (label))
    {
      SCIL_DECL_DATA (label) = scil_copy_stack ();
    }

  return build1 (GOTO_EXPR, void_type_node, label);
}


/* INSTRUCTIONS */

void 
scil_instr_branch (unsigned int target)
{
  scil_append_stmt (instr_goto (target));

  /* If the next instruction has not been targeted by forward branch yet,
     ECMA 335 requires its stack to be empty.  If it is has been the target
     of a forward branch, the current stack will be replaced by the right stack
     in scil_set_label.
  */
  scil_clear_stack ();
}

void 
scil_instr_branch_true (unsigned int target)
{
  tree conv_expr = pop_convert (scil_bool_type);
  tree goto_expr = instr_goto (target);
  tree cond_expr = build3 (COND_EXPR, void_type_node, conv_expr,
                           goto_expr, 
                           build_empty_stmt());
  scil_append_stmt (cond_expr);
}

void 
scil_instr_branch_false (unsigned int target)
{
  tree conv_expr = pop_convert (scil_bool_type);
  tree goto_expr = instr_goto (target);
  tree cond_expr = build3 (COND_EXPR, void_type_node, conv_expr,
                           build_empty_stmt(), 
                           goto_expr);
  scil_append_stmt (cond_expr);
}

void
scil_instr_compare_eq ()
{
  append_compare (EQ_EXPR);
}

void
scil_instr_compare_gt ()
{
  append_compare (GT_EXPR);
}

void
scil_instr_compare_lt ()
{
  append_compare (LT_EXPR);
}

void
scil_instr_compare_gt_un ()
{
  append_compare_un (GT_EXPR, UNGT_EXPR);
}

void
scil_instr_compare_lt_un ()
{
  append_compare_un (LT_EXPR, UNLT_EXPR);
}


void 
scil_instr_compare_ge ()
{
  append_compare (GE_EXPR);
}

void 
scil_instr_compare_le ()
{
  append_compare (LE_EXPR);
}

void 
scil_instr_compare_ge_un ()
{
  append_compare_un (GE_EXPR, UNGE_EXPR);
}

void 
scil_instr_compare_le_un ()
{
  append_compare_un (LE_EXPR, UNLE_EXPR);
}

void 
scil_instr_compare_ne_un ()
{
  append_compare_un (NE_EXPR, LTGT_EXPR);
}


void
scil_instr_dup ()
{
  scil_instr_push (scil_stack_top ());
}

void 
scil_instr_pop ()
{
  scil_stack_pop ();
}

void 
scil_instr_pop_into (tree variable)
{
  tree type, expr;

  variable = use_alias_if_any (variable);

  type = TREE_TYPE (variable);
  scil_append_stmt (build2 (MODIFY_EXPR, type, variable, pop_convert (type)));
}

void 
scil_instr_pop_into_field (tree field)
{
  tree ref_expr, expr, type, value;
  
  type = TREE_TYPE (field);
  value = pop_convert (type);

  ref_expr = instr_resolve_field (field);
  expr = build2 (MODIFY_EXPR, TREE_TYPE (ref_expr), ref_expr, value);

  scil_append_stmt (expr);
}

void 
scil_instr_push (tree variable)
{
  scil_append_stmt (instr_push (TREE_TYPE (variable), variable));
}

void 
scil_instr_push_field (tree field)
{
  scil_instr_push (instr_resolve_field (field));
}

void 
scil_instr_push_address (tree expr)
{
  tree type, push_expr;

  expr = use_alias_if_any (expr);

  type = TREE_TYPE (expr);
  type = build_pointer_type (type);

  push_expr = build1 (ADDR_EXPR, type, expr);
  TREE_ADDRESSABLE (expr) = 1;

  scil_append_stmt (instr_push (type, push_expr));
}

void 
scil_instr_push_field_address (tree field)
{
  scil_instr_push_address (instr_resolve_field (field));  
}


void 
scil_instr_push_virtual_method (unsigned int index)
{
  tree expr;

  expr = pop_convert (TYPE_POINTER_TO (scil_object_type));
  expr = scil_rt_get_vtable_content (expr, index);
  scil_instr_push (expr);
}


void 
scil_instr_push_integer (HOST_WIDE_INT constant, tree type)
{
  tree c = build_int_cst (type, constant);
  TREE_TYPE (c) = type;
  scil_append_stmt (instr_push (type, c));
}


void 
scil_instr_push_string (const char *constant, unsigned int length)
{
  static int string_index = 0;
  char name[16];  
  sprintf (name, SCIL_ID_STRING, string_index++);
  
  tree c = scil_make_string_constant (get_identifier (name), constant, length);
  scil_instr_push_address (c);
}


void 
scil_instr_call (tree method)
{
  tree parameters, expr, type;

  parameters = instr_pop_parameters (TYPE_ARG_TYPES (TREE_TYPE (method)));

  expr = build_function_call_expr (method, parameters);
  
  type = TREE_TYPE (expr);
  if (type != void_type_node)
    {
      expr = instr_push (type, expr);
    }

  scil_append_stmt (expr);
}


void 
scil_instr_callvirt (tree method, unsigned int index)
{
  tree parameters, expr, type, mtype, object;

  /* get parameters */
  mtype = TREE_TYPE (method);
  type = TREE_TYPE (mtype);
  parameters = instr_pop_parameters (TYPE_ARG_TYPES (mtype));
  
  /* replace implicit instance parameter by a SAVE_EXPR */
  object = TREE_VALUE (parameters);
  object = build1 (SAVE_EXPR, TREE_TYPE (object), object);
  TREE_VALUE (parameters) = object;
  
  /* build method call:  ((Object *) object).vtable [index] (object, params) */
  object = build1 (NOP_EXPR, TYPE_POINTER_TO (scil_object_type), object);
  expr = scil_rt_get_vtable_content (object, index);
  expr = build1 (NOP_EXPR, build_pointer_type (TREE_TYPE (method)), expr);
  expr = fold_build3 (CALL_EXPR, type, expr, parameters, NULL_TREE);
  
  /* return return value if any */
  if (type != void_type_node)
    {
      expr = instr_push (type, expr);
    }

  scil_append_stmt (expr);
}

void
scil_instr_call_indirect (tree method_type)
{
  tree parameters, expr, type;

  parameters = instr_pop_parameters (TYPE_ARG_TYPES (method_type));
  
  expr = pop_convert (build_pointer_type (method_type));
  expr = fold_build3 (CALL_EXPR, TREE_TYPE (method_type), expr, parameters, NULL_TREE);

  type = TREE_TYPE (expr);
  if (type != void_type_node)
    {
      expr = instr_push (type, expr);
    }

  scil_append_stmt (expr);
}


void
scil_instr_ret ()
{
  tree result_expr;
  if (!scil_stack_is_empty())
    {
      tree result_decl = DECL_RESULT (current_function_decl);
      tree type = TREE_TYPE (result_decl);
      result_expr = 
        build2 (MODIFY_EXPR, type, result_decl, pop_convert (type));
    }
  else
    {
      result_expr = NULL_TREE;
    }
  
  scil_append_stmt (build1 (RETURN_EXPR, void_type_node, result_expr));
}


void
scil_instr_newobj (tree constructor)
{
  tree object, parameters, type, vtable;
  
  /* alloc memory */ 
  type = DECL_CONTEXT (constructor);
  vtable = BINFO_VTABLE (TYPE_BINFO (type));

  object = scil_rt_new_object (type, vtable);

  /* leave out first argument ... */
  parameters = TREE_CHAIN (TYPE_ARG_TYPES (TREE_TYPE (constructor)));
  parameters = instr_pop_parameters (parameters);
  
  /* ... and add it to the head */
  parameters = tree_cons (NULL_TREE, object, parameters);

  /* call constructor */
  scil_append_stmt (build_function_call_expr (constructor, parameters));

  scil_instr_push (object);
}

void
scil_instr_new_static_delegate ()
{
  tree function_ptr;

  function_ptr = scil_stack_pop ();
  scil_stack_pop ();
  scil_append_stmt (instr_push (ptr_type_node, function_ptr));
}


void
scil_instr_add ()
{
  append_binop (PLUS_EXPR);
}

void
scil_instr_sub ()
{
  append_binop (MINUS_EXPR);
}

void
scil_instr_mul ()
{
  append_binop (MULT_EXPR);
}

void 
scil_instr_div ()
{
  append_binop_cases (TRUNC_DIV_EXPR, RDIV_EXPR);
}

void
scil_instr_div_un ()
{
  append_binop_un (TRUNC_DIV_EXPR);
}

void 
scil_instr_rem ()
{
  sorry ("rem");
  scil_append_stmt (error_mark_node);
}

void
scil_instr_rem_un ()
{
  append_binop_un (TRUNC_MOD_EXPR);
}

void 
scil_instr_and ()
{
  append_binop (BIT_AND_EXPR);
}

void 
scil_instr_or ()
{
  append_binop (BIT_IOR_EXPR);
}

void 
scil_instr_xor ()
{
  append_binop (BIT_XOR_EXPR);
}

void 
scil_instr_shl ()
{
  append_binop (LSHIFT_EXPR);
}

void 
scil_instr_shr ()
{
  append_binop (RSHIFT_EXPR);
}

void 
scil_instr_shr_un ()
{
  append_binop_un (RSHIFT_EXPR);
}

void 
scil_instr_neg ()
{
  append_unop (NEGATE_EXPR);
}

void 
scil_instr_not ()
{
  append_unop (BIT_NOT_EXPR);
}

void
scil_instr_convert (tree type)
{
  scil_append_stmt (instr_push (type, scil_stack_pop ()));
}


void
scil_append_stmt (tree stmt)
{
  tree *body = &BIND_EXPR_BODY (DECL_SAVED_TREE (current_function_decl));

  /* TODO: get these bits out of the file(s) we are parsing */
  annotate_with_file_line (stmt, main_input_filename, 0);
  append_to_statement_list_force (stmt, body);
}

