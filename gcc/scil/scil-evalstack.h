
extern tree scil_get_stack_variable (tree);
extern void scil_init_stacks (int);
extern void scil_free_stacks (void);
extern void scil_stack_push (tree);
extern tree scil_stack_pop (void);
extern tree scil_stack_top (void);
extern bool scil_stack_is_empty (void);
extern void *scil_copy_stack (void);
extern void scil_clear_stack (void);
extern void scil_set_stack (void *s);
