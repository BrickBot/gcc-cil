/* 
 * This file contains all the declarations that are necessary to plug
 * the SCIL front end in the main GCC.
 */

/* Hook function macros to let GCC know what our hooks are */

#undef  LANG_HOOKS_BUILTIN_FUNCTION 
#define LANG_HOOKS_BUILTIN_FUNCTION             scil_hook_builtin_function
#undef  LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION
#define LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION    tree_rest_of_compilation
#undef  LANG_HOOKS_FINISH 
#define LANG_HOOKS_FINISH                       scil_hook_finish
#undef  LANG_HOOKS_GETDECLS 
#define LANG_HOOKS_GETDECLS                     scil_hook_getdecls
#undef  LANG_HOOKS_GLOBAL_BINDINGS_P 
#define LANG_HOOKS_GLOBAL_BINDINGS_P            scil_hook_global_bindings_p
#undef  LANG_HOOKS_HANDLE_OPTION 
#define LANG_HOOKS_HANDLE_OPTION                scil_hook_handle_option
#undef  LANG_HOOKS_INIT 
#define LANG_HOOKS_INIT                         scil_hook_init
#undef  LANG_HOOKS_INIT_OPTIONS 
#define LANG_HOOKS_INIT_OPTIONS                 scil_hook_init_options
#undef  LANG_HOOKS_INSERT_BLOCK 
#define LANG_HOOKS_INSERT_BLOCK                 scil_hook_insert_block
#undef  LANG_HOOKS_MARK_ADDRESSABLE 
#define LANG_HOOKS_MARK_ADDRESSABLE             scil_hook_mark_addressable
#undef  LANG_HOOKS_PARSE_FILE 
#define LANG_HOOKS_PARSE_FILE                   scil_hook_parse_file
#undef  LANG_HOOKS_POST_OPTIONS 
#define LANG_HOOKS_POST_OPTIONS                 scil_hook_post_options
#undef  LANG_HOOKS_PUSHDECL 
#define LANG_HOOKS_PUSHDECL                     scil_hook_pushdecl
#undef  LANG_HOOKS_SIGNED_TYPE
#define LANG_HOOKS_SIGNED_TYPE                  scil_hook_signed_type
#undef  LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE 
#define LANG_HOOKS_SIGNED_OR_UNSIGNED_TYPE      scil_hook_signed_or_unsigned_type
#undef  LANG_HOOKS_TYPE_FOR_MODE
#define LANG_HOOKS_TYPE_FOR_MODE                scil_hook_type_for_mode
#undef  LANG_HOOKS_TYPE_FOR_SIZE
#define LANG_HOOKS_TYPE_FOR_SIZE                scil_hook_type_for_size
#undef  LANG_HOOKS_UNSIGNED_TYPE
#define LANG_HOOKS_UNSIGNED_TYPE                scil_hook_unsigned_type
#undef  LANG_HOOKS_WRITE_GLOBALS 
#define LANG_HOOKS_WRITE_GLOBALS                scil_hook_final_write_globals


/* Hook function prototypes */

tree scil_hook_builtin_function (const char *, tree, int, enum built_in_class, const char *, tree);
void scil_hook_final_write_globals (void);
void scil_hook_finish (void);
tree scil_hook_getdecls (void);
int  scil_hook_global_bindings_p (void);
int  scil_hook_handle_option (size_t, const char *, int);
bool scil_hook_init (void);
unsigned int scil_hook_init_options (unsigned int, const char **);
void scil_hook_insert_block (tree);
bool scil_hook_mark_addressable (tree);
void scil_hook_parse_file (int);
bool scil_hook_post_options (const char **);
tree scil_hook_pushdecl (tree);
tree scil_hook_signed_type (tree);
tree scil_hook_signed_or_unsigned_type (int, tree);
tree scil_hook_type_for_mode (enum machine_mode, int);
tree scil_hook_type_for_size (unsigned, int);
tree scil_hook_unsigned_type (tree);

#define SCIL_NO_COMPRESSION(node) TREE_LANG_FLAG_0(node)
#define SCIL_MEMORY_ALIAS_P(node) TREE_LANG_FLAG_1(node)


/* Hook data type intended to contain language specific data associated with 
   declarations in that language.
*/
struct lang_decl GTY(())
{
  tree mangling_chunk;
  tree unqualified_name;
  tree memory_alias;
  void * GTY((skip)) data;
};

#define SCIL_DECL_MANGLED(decl) (DECL_LANG_SPECIFIC (decl)->mangling_chunk)
#define SCIL_DECL_UNQUALIFIED(decl) (DECL_LANG_SPECIFIC (decl)->unqualified_name)
#define SCIL_DECL_MEMORY_ALIAS(decl) (DECL_LANG_SPECIFIC (decl)->memory_alias)
#define SCIL_DECL_DATA(decl) (DECL_LANG_SPECIFIC (decl)->data)

/* Hook data type intended to contain language specific data associated with 
   types in that language.
*/
struct lang_type GTY(())
{
  tree mangling_chunk;
  tree unqualified_name;
};

#define SCIL_TYPE_MANGLED(type) (TYPE_LANG_SPECIFIC (type)->mangling_chunk)
#define SCIL_TYPE_UNQUALIFIED(type) (TYPE_LANG_SPECIFIC (type)->unqualified_name)

#define SCIL_MANGLED(node) \
  (TYPE_P (node) ? SCIL_TYPE_MANGLED(node) : SCIL_DECL_MANGLED(node))
#define SCIL_UNQUALIFIED(node) \
  (TYPE_P (node) ? SCIL_TYPE_UNQUALIFIED(node) : SCIL_DECL_UNQUALIFIED(node))


/* Hook data type intended to contain language specific data associated with 
   functions in that language.
*/
struct language_function GTY(())
{
  char unused;
};

/* Hook data type intended to contain language specific data associated with 
   identifiers in that language.

   There are fully qualified identifiers for:
    * assemblies (eg [mscorlib])
    * namespaces (eg [mscorlib]System.Collections)
    * types (ie classes and interfaces) (eg [mscorlib]System.Collections.)
    * methods 
    * fields

   Assembly, namespace and type identifiers are unique when fully qualified. 
   Thus, we can use DECLARATION directly.

   Since method names can be overloaded, DECLARATION contains a TREE_LIST
   containing all methods with that name. If there is a field with that name
   it will be first in that list. TREE_VALUE is the method or field decl,
   TREE_PURPOSE is empty.

   There are further unqualified identifiers for:
    * labels
    * local variables
    * parameters
  
   These are local to the current method and thus DECLARATION can be used when 
   compiling the method they belong to.
   
   We also use identifiers for global VAR_DECLS for constant objects such as
   strings. In that case we make sure that the identifiers are unique such that
   the DECLARATION can be used directly.
   
   In fact, we try to read to use identifiers as rarely as possible since
   attaching each tree to the respective object in the CRAL is th more 
   efficient and cleaner way. Whenever the CIL code refers to a  decl the CRAL 
   has a reference to a CRAL object. Hence, we get the decl from there. 
   However, we store decls in DECLARATION to keep references on them that are 
   accessible for the GGC in order to force it not to throw them away.   
*/
struct lang_identifier GTY(())
{
  struct tree_identifier identifier;  
  tree declaration;
};

#define SCIL_DECL(identifier) \
          (((struct lang_identifier *) (identifier))->declaration)

/* Hook data type for all the tree node types that the front end uses.
*/
union lang_tree_node
GTY((desc ("TREE_CODE (&%h.generic) == IDENTIFIER_NODE"),
     chain_next ("(union lang_tree_node *)TREE_CHAIN (&%h.generic)")))
{
  union tree_node GTY((tag ("0"),
		       desc ("tree_node_structure (&%h)")))
    generic;
  struct lang_identifier GTY ((tag ("1"))) identifier;
};

