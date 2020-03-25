#ifndef CIL_INSTR_H
#define CIL_INSTR_H

/* dotgnu headers */
#include "program.h"

void instr_call (ILToken token);
void instr_callvirt (ILToken token);
void instr_newarr (ILToken token);
void instr_newobj (ILToken token);
void instr_ldfld (ILToken token);
void instr_ldftn (ILToken token);
void instr_ldsfld (ILToken token);
void instr_ldvirtftn (ILToken token);
void instr_stfld (ILToken token);
void instr_stsfld (ILToken token);
void instr_ldstr (ILToken token);
void instr_initobj (ILToken token);

#endif  /* CIL_INSTR_H */
