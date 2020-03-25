#ifndef CIL_MONO_STRUCTS_H
#define CIL_MONO_STRUCTS_H

/* cil-decl.h includes: config.h system.h tree.h */
#include "cil-decl.h"

#define MONO_TYPE_ATTRIBUTE_INTERFACE 0x00000020

#define MONO_DELEGATE_TARGET 4
#define MONO_DELEGATE_METHOD_PTR 6

void add_mono_obj_fields (tree decl);

void add_mono_array_fields (tree class_name);

void add_mono_string_fields (tree decl);

tree add_mono_class_fields (tree mono_class_type);

tree create_struct_mono_vtable (void);

tree create_struct_mono_class_runtime_info(void);

tree create_struct_mono_class_member_info(void);

#endif  /* CIL_MONO_STRUCTS_H */
