
/* appends STATEMENT to the currently compiled method */
extern void scil_append_stmt (tree statement);


/* Sets a label in the instruction list and returns that label.
   PROGRAM_COUNTER can be any value that identifies the label.
*/
extern void scil_set_label (unsigned int program_counter);

/* The various branching instructions. */
extern void scil_instr_branch (unsigned int target);
extern void scil_instr_branch_true (unsigned int target);
extern void scil_instr_branch_false (unsigned int target);

extern void scil_instr_compare_eq (void);
extern void scil_instr_compare_gt (void);
extern void scil_instr_compare_lt (void);
extern void scil_instr_compare_gt_un (void);
extern void scil_instr_compare_lt_un (void);

/* These functions do not exist but we included them for consistency reasons.
   That way, bge, ble, ... can be treated similarly to bgt, blt, ...
*/
extern void scil_instr_compare_ge (void);
extern void scil_instr_compare_le (void);
extern void scil_instr_compare_ge_un (void);
extern void scil_instr_compare_le_un (void);
extern void scil_instr_compare_ne_un (void);

/* Just pops the top from the stack. */ 
extern void scil_instr_pop (void);

/* Duplicates the top of the stack. */
extern void scil_instr_dup (void);

/* Pops VARIABLE from the virtual execution stack. This means, we generate code
   that moves the content of the variable representing the current stack 
   location into VARIABLE.
*/
extern void scil_instr_pop_into (tree variable);

/* Pops the next value and an object reference from the virtual execution stack
   and stores the value in the FIELD of the referenced object.
*/
extern void scil_instr_pop_into_field (tree field);

/* Pushes VARIABLE on the virtual execution stack. This means, we generate code
   that moves the content of VARIABLE in the variable representing the current
   stack location.
*/
extern void scil_instr_push (tree variable);

/* Pops an object reference from the virtual execution stack and pushes the
   content of the FIELD of that object on the stack.
*/
extern void scil_instr_push_field (tree field);

/* Pushes the address of VARIABLE on the virtual execution stack. This means, 
   we generate code that moves the address of VARIABLE in the variable 
   representing the current stack location.
*/
extern void scil_instr_push_address (tree variable);

/* Pops an object reference from the stack and pushes the address of the FIELD
   of that object on the stack.
*/
extern void scil_instr_push_field_address (tree variable);

extern void scil_instr_push_virtual_method (unsigned int index);

/* Pushes the CONSTANT of type TYPE on the stack. */
extern void scil_instr_push_integer (HOST_WIDE_INT constant, tree type);

/* Pushes the CONSTANT string on the stack. */
extern void scil_instr_push_string (const char *constant, unsigned int length);


/* Generates code for calling METHOD. */
extern void scil_instr_call (tree method);

/* Generates code for virtually calling METHOD. */
extern void scil_instr_callvirt (tree method, unsigned int index);

/* Generates code for calling a method of METHOD_TYPE a reference to which is 
   below its parameters on top of the stack. */
extern void scil_instr_call_indirect (tree method_type);

/* Generates code for returning from the current method. */
extern void scil_instr_ret (void);

/* Arithmetic operations as defined by ECMA 335*/
extern void scil_instr_add (void);
extern void scil_instr_sub (void);
extern void scil_instr_mul (void);
extern void scil_instr_div (void);
extern void scil_instr_div_un (void);
extern void scil_instr_rem (void);
extern void scil_instr_rem_un (void);
extern void scil_instr_and (void);
extern void scil_instr_or (void);
extern void scil_instr_xor (void);
extern void scil_instr_shl (void);
extern void scil_instr_shr (void);
extern void scil_instr_shr_un (void);
extern void scil_instr_neg (void);
extern void scil_instr_not (void);

/* Converts the value on top of the stack to TYPE */
extern void scil_instr_convert (tree type);

/* Creates a new object and calls CONSTRUCTOR for initialization. 
   The type of the new object is derived from CONSTRUCTOR.
*/
extern void scil_instr_newobj (tree constructor);


/* Creates a delegate the type of which has been declared with
   Hardware.StaticDelegateAttribute */
extern void scil_instr_new_static_delegate (void);
