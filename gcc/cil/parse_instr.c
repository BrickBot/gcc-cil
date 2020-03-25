/* GCC headers */
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "errors.h"
#include "real.h"

/* dotgnu headers */
#include "il_opcodes.h"

#include "build_trees.h"
#include "instr.h"
#include "parse_instr.h"
#include "parse_assembly.h"
#include "stack.h"

static tree get_label_name (unsigned long offset);
static unsigned long get_special_size (unsigned char *code, unsigned long csize);
static unsigned long parse_instr (unsigned char *code, unsigned long offset);
static tree parse_instr_arg (unsigned char *code, int argType, unsigned long offset);
static unsigned long parse_prefixed_instr (unsigned char *code, unsigned long offset);

tree
get_label_name (unsigned long offset)
{
  char *buf = alloca (11);
  sprintf (buf, "L_%08lX", offset);
  return get_identifier (buf);
}

unsigned long
get_special_size (unsigned char *code, unsigned long csize)
{
  unsigned long num_items;

  if (code[0] == (unsigned char)IL_OP_SWITCH)
    {
      /* Switch lookup table */
      if (csize < 5)
	{
	  return 0;
	}

      num_items = (unsigned long)(IL_READ_UINT32 (code + 1));

      if (num_items >= ((unsigned long)0x40000000) 
	  || (num_items * 4) > (csize - 5))
	{
	  return 0;
	}
      return num_items * 4 + 5;
    }
	
  return 0;
}

void
parse_instructions (ILMethodCode* method_code)
{
  unsigned char *code;
  unsigned long csize;
  unsigned long isize;
  unsigned long offset;
	
  /* a buffer with all IL instructions */
  code = (unsigned char *)method_code->code;
	
  /* size of the code buffer */
  csize = method_code->codeLen;
	
  offset = 0;

  while (csize > 0)
    {
      tree label_name = get_label_name (offset);
      make_label (label_name);

      if (*code == IL_OP_PREFIX)
	{
	  isize = parse_prefixed_instr (code, offset);
	}
      else
	{
	  isize = parse_instr (code, offset);
	}
		
      if (!isize)
	{
	  isize = get_special_size (code, csize);
	  
	  if (!isize)
	    {
	      internal_error ("Cannot determine instr size");
	    }
	}
		
      /* Move on to the next instruction */
      offset += isize;
      code += isize;
      csize -= isize;
    }
}

unsigned long
parse_instr (unsigned char *code, unsigned long offset)
{
  const ILOpcodeInfo *opcode;
  
  /* Extract the instruction from the method input stream */
  opcode = &(ILMainOpcodeTable[*code]);
  switch (*code)
    {
    case IL_OP_NOP:
      {
	break;
      }
    case IL_OP_BREAK:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDARG_0:
    case IL_OP_LDARG_1:
    case IL_OP_LDARG_2:
    case IL_OP_LDARG_3:
      {
	stack_push (formal_params[*code - IL_OP_LDARG_0]);
	break;
      }
    case IL_OP_LDLOC_0:
    case IL_OP_LDLOC_1:
    case IL_OP_LDLOC_2:
    case IL_OP_LDLOC_3:
      {
	stack_push (local_vars[*code - IL_OP_LDLOC_0]);
	break;
      }
    case IL_OP_STLOC_0:
    case IL_OP_STLOC_1:
    case IL_OP_STLOC_2:
    case IL_OP_STLOC_3:
      {
	make_assignment (local_vars[*code - IL_OP_STLOC_0]);
	break;
      }
    case IL_OP_LDARG_S:
      {
	int num = (int)(*(code + 1));
	stack_push (formal_params[num]);
	break;
      }
    case IL_OP_LDARGA_S:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STARG_S:
      {
	int num = (int)(*(code + 1));
	make_assignment (formal_params[num]);
	break;
      }
    case IL_OP_LDLOC_S:
      {
	int num = (int)(*(code + 1));
	stack_push (local_vars[num]);
	break;
      }
    case IL_OP_LDLOCA_S:
      {
	int num = (int)(*(code + 1));
	tree v = local_vars[num];
	stack_push (build1 (ADDR_EXPR,
			    build_pointer_type (TREE_TYPE (v)),
			    v));
	break;
      }
    case IL_OP_STLOC_S:
      {
	int num = (int)(*(code + 1));
	make_assignment (local_vars[num]);
	break;
      }
    case IL_OP_LDNULL:
      {
	stack_push (null_pointer_node);
	break;
      }
    case IL_OP_LDC_I4_M1:
      {
	tree num = build_int_cst (int32_type_node, -1);
	stack_push (num);
	break;
      }
    case IL_OP_LDC_I4_0:
    case IL_OP_LDC_I4_1:
    case IL_OP_LDC_I4_2:
    case IL_OP_LDC_I4_3:
    case IL_OP_LDC_I4_4:
    case IL_OP_LDC_I4_5:
    case IL_OP_LDC_I4_6:
    case IL_OP_LDC_I4_7:
    case IL_OP_LDC_I4_8:
      {
	tree num = build_int_cst (int32_type_node, *code - IL_OP_LDC_I4_0);
	stack_push (num);
	break;
      }
    case IL_OP_LDC_I4_S:
      {
	stack_push (parse_instr_arg (code,
				     opcode->args,
				     offset));
	break;
      }
    case IL_OP_LDC_I4:
      {
	stack_push (parse_instr_arg (code,
				     opcode->args,
				     offset));
	break;
      }
    case IL_OP_LDC_I8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDC_R4:
      {
	stack_push (parse_instr_arg (code,
				     opcode->args,
				     offset));
	break;
      }
    case IL_OP_LDC_R8:
      {
	stack_push (parse_instr_arg (code,
				     opcode->args,
				     offset));
	break;
      }
    case IL_OP_LDPTR:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_DUP:
      {
	make_instr_dup ();
	break;
      }
    case IL_OP_POP:
      {
	make_instr_pop ();
	break;
      }
    case IL_OP_JMP:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CALL:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_call (token);
	break;
      }
    case IL_OP_CALLI:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_RET:
      {
	make_return_value ();
	break;
      }
    case IL_OP_BR_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_unconditional (label_name);
	break;
      }
    case IL_OP_BRFALSE_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_on_false (label_name);
	break;
      }
    case IL_OP_BRTRUE_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_on_true (label_name);
	break;
      }
    case IL_OP_BEQ_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (EQ_EXPR, UNEQ_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BGE_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GE_EXPR, UNGE_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BGT_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GT_EXPR, UNGT_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BLE_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LE_EXPR, UNLE_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BLT_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LT_EXPR, UNLT_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BNE_UN_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	/* TODO: verify that NE_EXPR works both ordered
	   and unordered. */
	make_branch_comparison (NE_EXPR, NE_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BGE_UN_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GE_EXPR, UNGE_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BGT_UN_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GT_EXPR, UNGT_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BLE_UN_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LE_EXPR, UNLE_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BLT_UN_S:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LT_EXPR, UNLT_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BR:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_unconditional (label_name);
	break;
      }
    case IL_OP_BRFALSE:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_on_false (label_name);
	break;
      }
    case IL_OP_BRTRUE:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_on_true (label_name);
	break;
      }
    case IL_OP_BEQ:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (EQ_EXPR, UNEQ_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BGE:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GE_EXPR, UNGE_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BGT:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GT_EXPR, UNGT_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BLE:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LE_EXPR, UNLE_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BLT:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LT_EXPR, UNLT_EXPR,
				label_name, false);
	break;
      }
    case IL_OP_BNE_UN:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	/* TODO: verify NE_EXPR for unordered use */
	make_branch_comparison (NE_EXPR, NE_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BGE_UN:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GE_EXPR, UNGE_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BGT_UN:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (GT_EXPR, UNGT_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BLE_UN:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LE_EXPR, UNLE_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_BLT_UN:
      {
	tree label_name = parse_instr_arg (code,
					   opcode->args,
					   offset);
	make_branch_comparison (LT_EXPR, UNLT_EXPR,
				label_name, true);
	break;
      }
    case IL_OP_SWITCH:
      {
	tree label_names = parse_instr_arg (code,
					    opcode->args,
					    offset);
						
	make_instr_switch (label_names);
	break;
      }
    case IL_OP_LDIND_I1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_U1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_I2:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_U2:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_I4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_U4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_I8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_I:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_R4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_R8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDIND_REF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_REF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_I1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_I2:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_I4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_I8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_R4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_R8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ADD:
      {
	make_arithmetic_expr (PLUS_EXPR);
	break;
      }
    case IL_OP_SUB:
      {
	make_arithmetic_expr (MINUS_EXPR);
	break;
      }
    case IL_OP_MUL:
      {
	make_arithmetic_expr (MULT_EXPR);
	break;
      }
    case IL_OP_DIV:
      {
	if (TREE_CODE (TREE_TYPE (stack_top (NULL_TREE))) == REAL_TYPE)
	  make_arithmetic_expr (RDIV_EXPR);
	else
	  make_arithmetic_expr (TRUNC_DIV_EXPR);
	break;
      }
    case IL_OP_DIV_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_REM:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_REM_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_AND:
      {
	make_arithmetic_expr (BIT_AND_EXPR);
	break;
      }
    case IL_OP_OR:
      {
	make_arithmetic_expr (BIT_IOR_EXPR);
	break;
      }
    case IL_OP_XOR:
      {
	make_arithmetic_expr (BIT_XOR_EXPR);
	break;
      }
    case IL_OP_SHL:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_SHR:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_SHR_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_NEG:
      {
	make_unary_expr (NEGATE_EXPR);
	break;
      }
    case IL_OP_NOT:
      {
	make_unary_expr (BIT_NOT_EXPR);
	break;
      }
    case IL_OP_CONV_I1:
      {
	stack_convert_top_item (int8_type_node);
	break;
      }
    case IL_OP_CONV_I2:
      {
	stack_convert_top_item (int16_type_node);
	break;
      }
    case IL_OP_CONV_I4:
      {
	stack_convert_top_item (int32_type_node);
	break;
      }
    case IL_OP_CONV_I8:
      {
	stack_convert_top_item (int64_type_node);
	break;
      }
    case IL_OP_CONV_R4:
      {
	stack_convert_top_item (float_type_node);
	break;
      }
    case IL_OP_CONV_R8:
      {
	stack_convert_top_item (double_type_node);
	break;
      }
    case IL_OP_CONV_U4:
      {
	stack_convert_top_item (uint32_type_node);
	break;
      }
    case IL_OP_CONV_U8:
      {
	stack_convert_top_item (uint64_type_node);
	break;
      }
    case IL_OP_CALLVIRT:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_callvirt (token);
	break;
      }
    case IL_OP_CPOBJ:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDOBJ:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDSTR:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_ldstr (token);
	break;
      }
    case IL_OP_NEWOBJ:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_newobj (token);
	break;
      }
    case IL_OP_CASTCLASS:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ISINST:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_R_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_DATA_S:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNBOX:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_THROW:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDFLD:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_ldfld (token);
	break;
      }
    case IL_OP_LDFLDA:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STFLD:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_stfld (token);
	break;
      }
    case IL_OP_LDSFLD:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_ldsfld (token);
	break;
      }
    case IL_OP_LDSFLDA:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STSFLD:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_stsfld (token);
	break;
      }
    case IL_OP_STOBJ:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I1_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I2_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I4_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I8_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U1_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U2_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U4_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U8_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_BOX:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_NEWARR:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_newarr (token);
	break;
      }
    case IL_OP_LDLEN:
      {
	make_instr_ldlen ();
	break;
      }
    case IL_OP_LDELEMA:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDELEM_I1:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_U1:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_I2:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_U2:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_I4:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_U4:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_I8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDELEM_I:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDELEM_R4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDELEM_R8:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_LDELEM_REF:
      {
	stack_push (create_array_elem_access ());
	break;
      }
    case IL_OP_STELEM_I:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STELEM_I1:
      {
	make_array_elem_assignment ();
	break;
      }
    case IL_OP_STELEM_I2:
      {
	make_array_elem_assignment ();
	break;
      }
    case IL_OP_STELEM_I4:
      {
	make_array_elem_assignment ();
	break;
      }
    case IL_OP_STELEM_I8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STELEM_R4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STELEM_R8:
      {
	make_array_elem_assignment ();
	break;
      }
    case IL_OP_STELEM_REF:
      {
	make_array_elem_assignment ();
	break;
      }
    case IL_OP_LDELEM:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STELEM:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNBOX_ANY:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_A6:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_A7:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_A8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_A9:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_AA:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_AB:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_AC:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_AD:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_AE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_AF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_B0:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_B1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_B2:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I2:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U2:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_I8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U8:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_BB:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_BC:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_BD:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_BE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_BF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_C0:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_C1:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_REFANYVAL:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CKFINITE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_C4:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_UNUSED_C5:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_MKREFANY:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_CALL:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_CATCH:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_DEAD:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_HOISTED:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_HOISTED_CALL:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_LAB:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_DEF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_REF_S:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ANN_PHI:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LDTOKEN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_U2:
      {
	stack_convert_top_item (uint16_type_node);
	break;
      }
    case IL_OP_CONV_U1:
      {
	stack_convert_top_item (uint8_type_node);
	break;
      }
    case IL_OP_CONV_I:
      {
	stack_convert_top_item (native_int_type_node);
	break;
      }
    case IL_OP_CONV_OVF_I:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_OVF_U:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ADD_OVF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ADD_OVF_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_MUL_OVF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_MUL_OVF_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_SUB_OVF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_SUB_OVF_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_ENDFINALLY:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LEAVE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_LEAVE_S:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_STIND_I:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_OP_CONV_U:
      {
	stack_convert_top_item (native_uint_type_node);
	break;
      }
    default:
      {
	internal_error ("Unkown instruction: '%s'",
			opcode->name);
      }
    }

  if (opcode->size)
    {
      return opcode->size;
    }
  else
    {
      return 0;
    }
}

unsigned long
parse_prefixed_instr (unsigned char *code, unsigned long offset ATTRIBUTE_UNUSED)
{
  const ILOpcodeInfo *opcode;

  /* Extract the instruction from the method input stream */
  opcode = &(ILPrefixOpcodeTable[*(++code)]);
	
  switch (*code)
    {
    case IL_PREFIX_OP_ARGLIST:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_CEQ:
      {
	/* Puts 0 or 1 onto the stack */
	make_arithmetic_expr (EQ_EXPR);
	stack_convert_top_item (int32_type_node);
	break;
      }
    case IL_PREFIX_OP_CGT:
      {
	make_arithmetic_expr (GT_EXPR);
	stack_convert_top_item (int32_type_node);
	break;
      }
    case IL_PREFIX_OP_CGT_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_CLT:
      {
	make_arithmetic_expr (LT_EXPR);
	stack_convert_top_item (int32_type_node);
	break;
      }
    case IL_PREFIX_OP_CLT_UN:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_LDFTN:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_ldftn (token);
	break;
      }
    case IL_PREFIX_OP_LDVIRTFTN:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_ldvirtftn (token);
	break;
      }
    case IL_PREFIX_OP_JMPI:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_LDARG:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_LDARGA:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_STARG:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_LDLOC:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_LDLOCA:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_STLOC:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_LOCALLOC:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_UNUSED_PREFIX_10:
      {
	internal_error ("Unkown instruction: '%s'",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_ENDFILTER:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_UNALIGNED:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_VOLATILE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_TAIL:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_INITOBJ:
      {
	ILToken token = (ILToken)(IL_READ_UINT32 (code + 1));
	instr_initobj (token);
	break;
      }
    case IL_PREFIX_OP_ANN_LIVE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_CPBLK:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_INITBLK:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_ANN_REF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_RETHROW:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_UNUSED_PREFIX_1B		:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_SIZEOF:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    case IL_PREFIX_OP_REFANYTYPE:
      {
	internal_error ("Instruction '%s' is not supported yet",
			opcode->name);
	break;
      }
    default:
      {
	internal_error ("Unkown instruction: '%s'",
			opcode->name);
      }
    }
	
  if (opcode->size)
    {
      return opcode->size;
    }
  else
    {
      return 0;
    }
}

tree
parse_instr_arg (unsigned char *code, int argType, unsigned long offset)
{
  switch (argType)
    {
    case IL_OPCODE_ARGS_INVALID:
      {
	internal_error ("IL instruction argument type is invalid");
	break;
      }
    case IL_OPCODE_ARGS_NONE: break;
    case IL_OPCODE_ARGS_INT8:
      {
	char num = (char)(*(code + 1));
	tree t = build_int_cst (int32_type_node, num);
	return t;
      }
    case IL_OPCODE_ARGS_UINT8:
      {
	internal_error ("IL instruction argument type not implemented");
	break;
      }
    case IL_OPCODE_ARGS_INT16:
      {
	internal_error ("IL instruction argument type not implemented");
	break;

      }
    case IL_OPCODE_ARGS_UINT16:
      {
	internal_error ("IL instruction argument type not implemented");
	break;
      }
    case IL_OPCODE_ARGS_INT32:
      {
	ILInt32 num = IL_READ_INT32 (code + 1);
	tree t = build_int_cst (int32_type_node, num);
	return t;
      }
    case IL_OPCODE_ARGS_FLOAT32:
      {
	ILInt64 num = IL_READ_INT64 (code + 1);
	REAL_VALUE_TYPE val;
	tree t;

	real_from_target_fmt (&val, (long*)&num, &ieee_single_format);
	t = build_real (float_type_node, val);
	return t;
      }
    case IL_OPCODE_ARGS_FLOAT64:
      {
	ILInt64 num = IL_READ_INT64 (code + 1);
	REAL_VALUE_TYPE val;
	tree t;

	real_from_target_fmt (&val, (long*)&num, &ieee_double_format);
	t = build_real (double_type_node, val);
	return t;
      }
    case IL_OPCODE_ARGS_INT64:
    case IL_OPCODE_ARGS_TOKEN:
    case IL_OPCODE_ARGS_NEW:
    case IL_OPCODE_ARGS_LDTOKEN:
    case IL_OPCODE_ARGS_SHORT_VAR:
    case IL_OPCODE_ARGS_SHORT_ARG:
    case IL_OPCODE_ARGS_LONG_VAR:
    case IL_OPCODE_ARGS_LONG_ARG:
    case IL_OPCODE_ARGS_ANN_DEAD:
    case IL_OPCODE_ARGS_ANN_LIVE:
    case IL_OPCODE_ARGS_ANN_ARG:
      {
	internal_error ("IL instruction argument type not implemented");
	break;
      }
    case IL_OPCODE_ARGS_SHORT_JUMP:
      {
	unsigned long dest;
	dest = (unsigned long)(((long)offset)
			       + 2
			       + (long)(ILInt8)(*(code + 1)));
	return get_label_name (dest);
      }
    case IL_OPCODE_ARGS_LONG_JUMP:
      {
	unsigned long dest;
	dest = (unsigned long)(((long)offset)
			       + 5
			       + (long)(IL_READ_UINT32(code + 1)));
	return get_label_name (dest);
      }
    case IL_OPCODE_ARGS_CALL:
    case IL_OPCODE_ARGS_CALLI:
    case IL_OPCODE_ARGS_CALLVIRT:
      {
	internal_error ("IL instruction argument type not implemented");
	break;
      }
    case IL_OPCODE_ARGS_SWITCH:
      {
	tree labels = NULL_TREE;
	unsigned long dest, item, num_items;
			
	num_items = (unsigned long)(IL_READ_UINT32 (code + 1));

	for (item = 0; item < num_items; ++item)
	  {
	    dest = (unsigned long)(((long)offset)
				   + 5
				   + ((long)num_items) * 4
				   + (long)(IL_READ_UINT32 (
							    code + 5 + item * 4)));
							
	    labels = chainon (labels,
			      build_tree_list (
					       build_int_cst (integer_type_node, 
							      item),
					       get_label_name (dest)));
	  }
			
	return labels;
      }
    case IL_OPCODE_ARGS_STRING:
    case IL_OPCODE_ARGS_ANN_DATA:
    case IL_OPCODE_ARGS_ANN_REF:
    case IL_OPCODE_ARGS_ANN_PHI:
      {
	internal_error ("IL instruction argument type not implemented");
	break;
      }
    default:
      {
	internal_error ("Unknown IL instruction argument type");
	break;
      }
    }
  return NULL_TREE;
}
