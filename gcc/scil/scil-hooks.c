/* 
 * This file contains all the definitions that are necessary to plug
 * the SCIL front end in the main GCC.
 *
 * This file must contain #include directives for _all_ header files 
 * that use GTY markers in order to make the GGC work correctly!
 */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"

#include "ggc.h" /* for gtype-scil.h */
#include "toplev.h"

#include "convert.h"

#include "langhooks.h"
#include "langhooks-def.h"

#include "scil-hooks.h"
#include "scil-builtins.h"

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) TYPE,

const enum tree_code_class tree_code_type[] = {
#include "tree.def"
};
#undef DEFTREECODE

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) LENGTH,

const unsigned char tree_code_length[] = {
#include "tree.def"
};
#undef DEFTREECODE

#define DEFTREECODE(SYM, NAME, TYPE, LEN) NAME,

const char *const tree_code_name[] = {
#include "tree.def"
};
#undef DEFTREECODE

const struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;
  

/* Take those functions out of here and to the appropriate places. */

static tree
conversion_error (tree type, tree expr)
{
  sorry ("Conversion from type %s to type %s", 
                  type, TREE_TYPE (expr));
  return error_mark_node;
}


/* Create an expression whose value is that of EXPR,
   converted to type TYPE.  The TREE_TYPE of the value
   is always TYPE.  This function implements all reasonable
   conversions; callers should filter out those that are
   not permitted by the language being compiled.  */
tree
convert (tree type, tree expr)
{
  switch (TREE_CODE (type))
    {
    case INTEGER_TYPE:
      return fold (convert_to_integer (type, expr));
    
    case BOOLEAN_TYPE:
      {
      tree other;

      switch (TREE_CODE (TREE_TYPE (expr)))
	{
	case INTEGER_TYPE:
	  other = integer_zero_node;
	  break;
        
	case POINTER_TYPE:
	  other = null_pointer_node;
	  break;
        
	case BOOLEAN_TYPE:
	  return expr;
        
	default:
          return conversion_error (type, expr);
	}
      return fold (build (NE_EXPR, boolean_type_node, expr, other));
      }
    case POINTER_TYPE:
      return fold (convert_to_pointer (type, expr));

    case REAL_TYPE:
      return fold (convert_to_real (type, expr));

    default:
      return conversion_error (type, expr);
    }
}


/* see lang_hooks_for_decls.getdecls in langhooks.h */
tree
scil_hook_getdecls (void)
{
  sorry ("scil_hook_getdecls");
  return error_mark_node;
}

/* see lang_hooks_for_decls.global_bindings_p in langhooks.h */
int
scil_hook_global_bindings_p (void)
{
  sorry ("scil_hook_global_bindings_p");
  return 0;
}

/* see lang_hooks_for_decls.insert_block in langhooks.h */
void
scil_hook_insert_block (tree block ATTRIBUTE_UNUSED)
{
  sorry ("scil_hook_insert_block");
}

/* see lang_hooks.mark_addressable in langhooks.h */
bool
scil_hook_mark_addressable (tree expr)
{
  switch (TREE_CODE (expr))
   {
   case VAR_DECL:
     return false;
   default:
     TREE_ADDRESSABLE (expr) = 1;
     return true;
   }
}


/* see lang_hooks_for_types.signed_type in langhooks.h */
tree 
scil_hook_signed_type (tree type)
{
  switch (TYPE_PRECISION (type))
    {
    case 8:
      return scil_int8_type;
    
    case 16:
      return scil_int16_type;
    
    case 32:
      return scil_int32_type;

    case 64:
      return scil_int64_type;

    default:
      sorry ("scil_hook_signed_type for type with %d bits precision", TYPE_PRECISION (type));
      return error_mark_node;
    }
}

/* see lang_hooks_for_types.signed_or_unsigned_type in langhooks.h */
tree 
scil_hook_signed_or_unsigned_type (int unsigned_p, tree type)
{
  return unsigned_p ? 
           scil_hook_unsigned_type (type) : 
           scil_hook_signed_type (type);
}

/* see lang_hooks_for_types.unsigned_type in langhooks.h */
tree 
scil_hook_unsigned_type (tree type)
{
  switch (TYPE_PRECISION (type))
    {
    case 8:
      return scil_uint8_type;
    
    case 16:
      return scil_uint16_type;
    
    case 32:
      return scil_uint32_type;

    case 64:
      return scil_uint64_type;

    default:
      sorry ("scil_hook_unsigned_type for type with %d bits precision", TYPE_PRECISION (type));
      return error_mark_node;
    }
}

/* see lang_hooks_for_types.type_for_mode in langhooks.h */
tree
scil_hook_type_for_mode (enum machine_mode mode, int unsigned_p)
{
  /* These modes are defined insn-modes.h in the binary directory. */
  /* We can use the modes directly because they are fixed size like 
     our primitive types. */
  switch (mode)
    {
    case QImode:
      return unsigned_p ? scil_uint8_type : scil_int8_type;
    
    case HImode:
      return unsigned_p ? scil_uint16_type : scil_int16_type;

    case SImode:
      return unsigned_p ? scil_uint32_type : scil_int32_type;

    case TImode:
      return unsigned_p ? scil_uint64_type : scil_int64_type;
    
    case BImode:
      return scil_bool_type;
    
    case VOIDmode:
      return void_type_node;
    
    default:
      break;
    }
  
  /* TODO: return appropriate complex types */
  /* Returning NULL_TREE here (instead of calling sorry) because
     build_common_builtin_nodes in tree.c can work that way. */
  if (COMPLEX_MODE_P (mode))
    return NULL_TREE;
  
  sorry ("scil_hook_type_for_mode (mode: %d, unsigned_p: %d)", mode, unsigned_p);
  return error_mark_node;
}

/* see lang_hooks_for_types.type_for_size in langhooks.h */
tree
scil_hook_type_for_size (unsigned precision, int unsigned_p)
{
  if (precision <= 8)
      return unsigned_p ? scil_uint8_type : scil_int8_type;

  if (precision <= 16)
      return unsigned_p ? scil_uint16_type : scil_int16_type;

  if (precision <= 32)
      return unsigned_p ? scil_uint32_type : scil_int32_type;

  if (precision <= 64)
      return unsigned_p ? scil_uint64_type : scil_int64_type;
  
  sorry ("scil_hook_type_for_size (precision: %d, unsigned_p: %d)", precision, unsigned_p);
  return error_mark_node;
}


#include "debug.h"
#include "gtype-scil.h"
