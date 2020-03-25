#ifndef CIL_PNET_ADAPT_H
#define CIL_PNET_ADAPT_H

#include "program.h"

#define	ILClass_IsBeforeFieldInit(info)	\
	((ILClassGetAttrs((info)) & IL_META_TYPEDEF_BEFORE_FIELD_INIT) != 0)


const char *get_assembly_ref_name (ILClass *class_ref);

ILProgramItem* get_class_ref_scope (ILClass *class_ref);

ILClass* resolve_class_ref (ILContext* context, ILClass* class_ref);

void load_assembly (ILContext* context, ILImage** image, const char *filename);

void load_mscorlib (ILContext* context);


#endif /* CIL_PNET_ADAPT_H */
