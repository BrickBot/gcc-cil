#ifndef CIL_MANGLE_H
#define CIL_MANGLE_H

void cil_init_mangle (void);
tree mangle_function (ILMethod*);
tree mangle_class_data (ILClass *klass);
tree mangle_mono_class (ILClass*);
tree mangle_vtable (ILClass*);
tree mangle_mono_class_runtime_info (ILClass*);

#endif
