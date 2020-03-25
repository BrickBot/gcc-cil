/* The CIL front end callbacks for the CIL Reader Adaption Layer. 
 *
 * Contains declarations of callback functions that are intended to be called
 * by the CRAL front end.
 */

#define SCIL_POINTER_TYPE(TYPE) build_pointer_type (TYPE)
#define SCIL_TYPE_PARSED(TYPE) COMPLETE_TYPE_P (TYPE)
#define SCIL_VTABLE_PARSED(TYPE) BINFO_VTABLE (TYPE_BINFO (TYPE)) != NULL_TREE


/* defined in scil-class.c */


/* Returns a NAMESPACE_DECL tree for the given NAME that can be used as a 
   container for scil_register_namespace or scil_register_class.
*/
extern tree scil_register_assembly (const char *name);

/* Returns a NAMESPACE_DECL tree for the given NAME that can be used as a 
   container for scil_register_namespace or scil_register_class. The name of 
   CONTAINER will be prepended.
   This function automatically splits the '.'-separated parts of the namespace
   into several trees that form a namespace hierarchy. 
   Eg, with the CONTAINER name "mscorlib" and the namespace "System.Collections"
   this function will generate two trees: one with name "mscorlib.System" and 
   one with name "mscorlib.System.Collections", the former being the 
   DECL_CONTEXT of the latter. The latter will be returned.
   
*/
extern tree scil_register_namespace (tree container, const char *name);


/* Tells the front end that a class with NAME and is defined in the scope of
   CONTAINER, which may either be class, an assembly or a namespace decl.
   The class's full name is defined by CONTAINER name + '.' + NAME. Hence, this
   function automatically resolves namespaces that are included in the class
   name using scil_register_namespace.
*/
extern tree scil_register_class (tree container, const char *name);

/* Adds a non-static field  with NAME and of FIELD_TYPE to TYPE. */
extern tree scil_add_field (tree type, const char *name, tree field_type);

/* Sets the BASE_CLASS for TYPE. */
extern void scil_set_base_class (tree type, tree base_class);

/* Finishes off the non-static fields part of TYPE. */
extern void scil_complete_type (tree type);

/* Registers a static field of FIELD_TYPE and with NAME, which has been declared in 
   TYPE. The complete name will be constructed from TYPE's name and NAME.
*/
extern tree scil_register_static_field (tree type, const char *name, tree field_type);

/* Makes the virtual-method table for TYPE which has SIZE new virtual methods. */
extern void scil_make_vtable (tree type, unsigned int size);

/* Sets the address of METHOD in TYPE's vtable at INDEX. */
extern void scil_set_vtable_slot (tree type, unsigned int index, tree method);

/* Returns the index of a new slot in TYPE's vtable. */
extern unsigned int scil_new_vtable_slot (tree type);

/* Finishes off the definition of TYPE's vtable. */
extern void scil_complete_vtable (tree type);

/* defined in scil-method.c */

/* For declaring a method you have to do the following steps in that order:

   * make a method type (scil_make_method_type)
   * add all parameter types to that type (scil_add_method_parameter_type)
   * complete the method type (scil_complete_method_type)
   * register the method with that type (scil_register_method)
   
   For defining a method you further have to:
  
   * begin the definition (scil_start_method_def)
   * add all parameters (scil_add_method_parameter)
   * add all local variables (scil_add_method_variable)
   * add all statements (scil_stmt_*)
   * initiate the method's compilation (scil_complete_method)
*/

/* Creates a preliminary tree for a method type whose return type is denoted by
   RETURN_TYPE. The CRAL is supposed to add the parameter types to this type
   using scil_add_method_paramter_type for each parameter and call 
   scil_complete_method_type once all parameter types have been added.
   The parameter names are added later when the method is defined.
   The CRAL should not add the implicit ``this'' parameter, this is done 
   automatically be scil_complete_method_type if appropriate,   
*/
extern tree scil_make_method_type (tree return_type);

/* Adds a parameter of TYPE to the METHOD_TYPE. If BYREF is true, the 
   parameter's type will be a pointer type to TYPE.
*/
extern void scil_add_method_parameter_type (tree method_type, tree type, bool byref);

/* Finishes the definition of METHOD_TYPE. After that call no parameters should
   be added. CLASS_TREE = NULL_TREE indicates a static method, otherwise it is
   the class that defines this method. In this case, an implicit argument of 
   that type is added to the front of the parameter list automatically.
   VARIABLE_ARGUMENT_LIST if true indicates an open-length  parameter list as 
   with eg System.Console.WriteLine.
   Returns the finished type which might be different from METHOD_TYPE.
*/
extern tree scil_complete_method_type (tree method_type, tree class_tree,
                                       bool variable_argument_list);


/* Tells the front end that a method with name METHOD_NAME exists in CONTAINER.
   This function only registers a method definition. For virtual methods, you
   must also add a slot to the class.
   RUNTIME is non-zero if this method is provided by the runtime. If it is zero
   it can still be a runtime method in case the method has been registered as a
   runtime method before.
   If MANGLE_NAME is non-zero, this function will mangle the name and set the
   assembler name to this name.
*/
extern tree scil_register_method_impl (tree container, 
                                       const char *name, 
                                       tree method_type, 
                                       int runtime, 
                                       int mangle_name); 

#define scil_register_method(container, name, type) \
  scil_register_method_impl (container, name, type, 0, 1)

/* Declares a method as entry point. */
extern void scil_set_entry_point (tree method);

/* Starts the definition process of METHOD. current_function_decl is set to
   METHOD until the next call to this function. All calls to 
   scil_add_method_{parameter, variable} and scil_complete_method_def as well 
   as to all functions in scil-instruction.h refer to METHOD implicitly (via
   current_function_decl). MAX_STACK is the maximum height of the stack during
   the compilation of METHOD.
   Returns false if METHOD has already been defined. In this case, none of the
   functions mentioned above shall be called.
*/
extern bool scil_start_method_def (tree method, int max_stack);

/* Adds the next parameter with NAME to the method currently compiled. Returns 
   the created tree for that parameter.
*/
extern tree scil_add_method_parameter (const char *name);

/* Adds a local variable of TYPE to the METHOD. Returns the created tree for 
   that variable.
*/
extern tree scil_add_method_variable (tree type);

/* Adds a stack variable of TYPE to the METHOD. Returns the created tree for 
   that variable.
*/
extern tree scil_add_stack_variable (tree type);


/* Finishes the method's definition and compiles the method. */
extern void scil_complete_method_def (void);

/* Adds a memory alias to FIELD. ADDRESS is the relative address as specified
   by the respective CLI attribute. Absolute addresses are said to be relative
   to 0.
*/
extern void scil_add_memory_alias (tree field, HOST_WIDE_INT address);

/* Declares METHOD_DEF to be a class constructor. This will force execution
   of this method at start-up.
*/
extern void scil_mark_class_constructor (tree method_def);

/* Adds a GCC attribute to the ITEM. */
extern void scil_add_attribute (tree item, const char *attr);
