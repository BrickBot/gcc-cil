/* Creates alle the builtin types and functions that the front end and the GCC
   need. 
*/
extern void scil_make_builtins (void);

/* Adds a statement to the real int main(int, char **) function which serves as
   a wrapper for the program's .entrypoint method and additional initialization.
*/
extern void scil_append_statement_to_main (tree statement);


/* Primitive data types */

extern tree scil_bool_type;
extern tree scil_char_type;

extern tree scil_object_type;
extern tree scil_string_type;
extern tree scil_string_literal_type;

extern tree scil_float32_type;
extern tree scil_float64_type;

extern tree scil_int8_type;
extern tree scil_int16_type;
extern tree scil_int32_type;
extern tree scil_int64_type;

extern tree scil_native_int_type;
extern tree scil_native_uint_type;

extern tree scil_typed_ref;

extern tree scil_uint8_type;
extern tree scil_uint16_type;
extern tree scil_uint32_type;
extern tree scil_uint64_type;


/* Basic attributes */

extern tree scil_attribute_static;
extern tree scil_attribute_public;
extern tree scil_attribute_protected;
extern tree scil_attribute_private;
