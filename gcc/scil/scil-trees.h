
/* Builds the full name of the class and returns the tree from get_identifier.
   If NAME_SPACE is not empty, the full name of the class is '[' + ASSEMBLY + 
   ']' + NAME_SPACE + '.' + CLASS_NAME. Otherwise it is '[' + ASSEMBLY + ']' +
   CLASS_NAME.
*/
extern tree scil_get_full_class_identifier (const char *assembly, 
                                            const char *name_space,
                                            const char *class_name);

/* Returns the tree from get_identifier (PARENT_ID + '.' + CHILD) */
extern tree scil_get_nested_identifier (tree parent_id, const char *child);

/* Creates a new empty record. */
extern tree scil_make_record (void);

/* 
  Returns the name of NODE which can either be a decl or a type.
*/
extern tree scil_get_id (tree node);

/* Adds the initialization of FIELD with VALUE to the CONSTRUCTOR. It is the
   callers responsibility to make sure that the field is actually the next 
   field of the type and that this function is called for all fields. */
extern void scil_init_next_field (tree constructor, tree field, tree value);

/* Creates a new string object that is known at compile time and returns a 
   runtime reference to it.
*/
extern tree scil_make_string_constant (tree id, const char *content, unsigned int length);

/* Wrapper and replacement for build_decl, allowing for language specific parts
*/
extern tree scil_build_decl (enum tree_code code, tree id, tree type);

/* Makes a new array constant with ID. It will contain LENGTH uninitialized 
   elements of type ELEMENT_TYPE. You need to add each element afterwards
   using scil_add_array_constant_element (in order to get its index) and set
   the element's value with scil_set_array_constant_element.
*/
extern tree scil_make_array_constant (tree id, tree element_type, unsigned int length);

/* Makes a new array constant with ID. It reserves LENGTH uninitialized 
   elements of type ELEMENT_TYPE. The first elements are copied from SOURCE.
   For all remaining elements, you need to add each element afterwards using 
   scil_add_array_constant_element (in order to get its index) and set the 
   element's value with scil_set_array_constant_element.
*/
extern tree scil_make_array_constant_with_init (tree id, tree element_type, unsigned int length, tree source);

/* Returns the number of elements in this ARRAY. */
extern unsigned int scil_get_array_constant_length (tree array);

/* Sets the element in this ARRAY at INDEX to VALUE. */
extern void scil_set_array_constant_element (tree array, unsigned int index, tree value);

/* Gets the element in this ARRAY at INDEX. */
extern tree scil_get_array_constant_element (tree array, unsigned int index);

/* Adds another slot to this ARRAY. You cannot add more elements than specified
   when creating the array. This is not checked and will result in strange run
   time behavior.
*/
extern unsigned int scil_add_array_constant_element (tree array);

/* Returns base class of TYPE or NULL_TREE if it has none. */
extern tree scil_base_class (tree type);

/* Returns true if TYPE is a subclass of BASE_TYPE or TYPE is BASE_TYPE.
*/
extern bool scil_derives_from (tree type, tree base_type);


