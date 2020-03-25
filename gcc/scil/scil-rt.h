/* Function declarations for the runtime specific parts. */

/* Initializes the object type as necessary for the runtime.
*/
extern void scil_rt_init_object_type (void);

/* Initializes the object type as necessary for the runtime.
*/
extern void scil_rt_init_string_type (void);


extern tree scil_rt_make_vtable (tree type, unsigned int size, tree base_vtable);
extern void scil_rt_set_vtable_slot (tree vtable, unsigned int index, tree address);
extern unsigned int  scil_rt_new_vtable_slot (tree vtable);
extern tree scil_rt_get_vtable_content (tree vtable, unsigned int index);

extern void scil_rt_init_constant_object (tree constructor, tree rt_fields, tree type);

extern void scil_rt_init_constant_string (tree constructor, 
                                          tree rt_fields,
                                          tree string_literal,
                                          unsigned int length);
                                          
extern tree scil_rt_new_object (tree type, tree vtable);


extern void scil_rt_make_builtins (void);
