#include <stddef.h>

#ifdef __GNUC
#define MONO_ZERO_LEN_ARRAY 0
#else
#define MONO_ZERO_LEN_ARRAY 1
#endif

#define TYPE_ATTRIBUTE_INTERFACE 0x00000020

#define MONO_CLASS_IS_INTERFACE(c) (c->flags & TYPE_ATTRIBUTE_INTERFACE)

typedef struct _MonoClass MonoClass;
typedef struct _MonoVTable MonoVTable;
typedef struct _MonoThreadsSync MonoThreadsSync;

/* This struct collects the info needed for the runtime use of a class,
 * like the vtables for a domain, the GC descriptor, etc.
 */
typedef struct
{
  unsigned short max_domain;
  /* domain_vtables is indexed by the domain id and the size is max_domain + 1 */
  MonoVTable *domain_vtables [MONO_ZERO_LEN_ARRAY];
} MonoClassRuntimeInfo;

struct _MonoClass
{
    unsigned int inited : 1;
    MonoClass *parent;
    unsigned short interface_count; 
    unsigned short interface_id;
    unsigned short max_interface_id;  
    int *interface_offsets;   
    MonoClass **interfaces;
    
    int instance_size;
    int vtable_size;
    
    unsigned int flags;

        struct {
                unsigned int first, count;
        } field, method, property, event;
        
    MonoClassRuntimeInfo *runtime_info;
};

struct _MonoVTable
{
  MonoClass *klass;
  void *gc_desc;
  void *domain;
  void *data;
  void *type;
  unsigned short max_interface_id;
  unsigned char rank;
  unsigned int remote : 1; /* class is remotely activated */
  unsigned int initialized : 1; /* cctor has been run */
  void *vtable[MONO_ZERO_LEN_ARRAY];
};

typedef struct
{
  MonoVTable *vtable;
  MonoThreadsSync *synchronisation;
} MonoObject;

typedef struct MonoArrayBounds MonoArrayBounds;

typedef struct {
	MonoObject obj;
	/* bounds is NULL for szarrays */
	MonoArrayBounds *bounds;
	/* total number of elements of the array */
	size_t max_length;
	/* we use double to ensure proper alignment on platforms that need it */
	double vector [MONO_ZERO_LEN_ARRAY];
} MonoArray;

typedef struct {
	MonoObject obj;
	size_t length;
	unsigned short chars[0];
} MonoString;

extern out_of_memory (void);
