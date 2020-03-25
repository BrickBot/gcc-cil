#ifndef CIL_PARSE_ASSEMBLY_H
#define CIL_PARSE_ASSEMBLY_H

#include "program.h"

/* ILIMAGE, which is currently parsed */
extern ILImage *c_image;

tree get_namespace_class_name (ILClass *klass);
tree get_class_decl (ILClass* klass);
void parse_assembly (const char *filename);
tree parse_parameters_types (ILMethod *method);
tree parse_return_type (ILMethod *method);
tree parse_type(ILType *type);


#endif  /* CIL_PARSE_ASSEMBLY_H */

