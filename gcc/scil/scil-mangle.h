/* Function declarations for name mangling in the SCIL frontend for the GCC. */

extern void scil_init_mangling (void);

extern tree scil_mangle_method (tree decl);

extern tree scil_mangle_field (tree decl);

extern tree scil_mangle_vtable (tree type);

extern tree scil_get_label_id (tree method, const char *name);
