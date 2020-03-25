/* The CIL Reader Adaption Layer. 
 *
 * Contains declarations of adapter functions that are intended to connect the
 * front end to the CIL Reader library.
 */

/* Initializes the CRAL. Returns true if done successfully. */
extern bool cral_init (void);

/* Sets the directory paths used when searching for assemblies and other
   resources. */
extern void cral_set_search_directories (int num, char **dirs);

/* Loads the system library (e.g. mscorlib.dll). */
extern void cral_load_system_library (const char *name);

/* Parses the input file. */
extern void cral_parse_file (const char *name);


/* Called to allow the CRAL to clean up. */
extern void cral_finalize (void);
