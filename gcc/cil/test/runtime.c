#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __GNUC
#define MONO_ZERO_LEN_ARRAY 0
#else
#define MONO_ZERO_LEN_ARRAY 1
#endif

#define TYPE_ATTRIBUTE_INTERFACE 0x00000020

#define MONO_CLASS_IS_INTERFACE(c) (c->flags & TYPE_ATTRIBUTE_INTERFACE)

typedef struct _MonoThreadsSync MonoThreadsSync;
typedef struct _MonoClass MonoClass;
typedef struct MonoVTable MonoVTable;
typedef struct _MonoObject MonoObject;

/* This struct collects the info needed for the runtime use of a class,
 * like the vtables for a domain, the GC descriptor, etc.
 */
typedef struct {
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

struct _MonoObject{
    MonoVTable *vtable; 
    MonoThreadsSync *synchronisation;
};

struct MonoVTable
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


struct MonoString{
  MonoObject obj;
  int len;
  unsigned short chars[];
};

typedef struct {
        MonoObject obj;
        void *bounds;
        size_t max_length;
        double vector [0];
} MonoArray;


void index_out_of_range()
{
  printf("index out of range\n");
  exit(1);
}

void null_reference()
{
  printf("null reference\n");
  exit(1);
}

void out_of_memory()
{
  printf("out of memory\n");
  exit(1);
}

void
init_class(MonoClass *klass);
void
init_interface(MonoClass *klass);
void 
create_runtime_vtable(MonoClass *klass);


void*
newobj (MonoClass *klass)
{
        MonoObject *obj = (MonoObject*) malloc(klass->instance_size);

        if (obj == 0)
        {
                out_of_memory();
        }

        // TODO: AppDomains are not supported yet, so there is only one vtable
        if (!klass->inited)
        {
                init_class(klass);
                create_runtime_vtable(klass);
        }
        obj->vtable = klass->runtime_info->domain_vtables[0];
        obj->synchronisation = 0;

        memset((char*) obj + sizeof(MonoObject), 0, klass->instance_size - sizeof(MonoObject));
        
        return obj;
}

void
initobj (void *obj, size_t size)
{
	memset(obj, 0, size);
}

void*
newarr (size_t length, size_t elem_size)
{
        MonoArray *array =
                (MonoArray*) malloc(length * elem_size
                                     + sizeof(MonoArray));

        if (array == 0)
        {
                out_of_memory();
        }

        array->obj.vtable = 0;
        array->obj.synchronisation = 0;
        array->bounds = 0;
        array->max_length = length;

        memset((char*) &array->vector, 0, length * elem_size);

        return array;
}

/* function is used for debugging purposes only */
void*
lookup_intf_method_ptr(MonoObject* this_ptr, MonoClass* intf, int index)
{
  unsigned short intf_id = intf->interface_id;
  void** interface_vtable_slot_ptr = *( (void***)this_ptr->vtable - (intf_id + 1) );
  void* intf_method_ptr = *(interface_vtable_slot_ptr + index);

  return intf_method_ptr;
}

short get_next_interface_id()
{
        static short id = 0;
        return id++;
}


int
count_implemented_ifaces (MonoClass *klass)
{
        int i, num_ifaces;
        MonoClass *ic;
    
        num_ifaces = 0;
        for (i = 0; i < klass->interface_count; i++) {
                ic = klass->interfaces [i];
                num_ifaces += count_implemented_ifaces (ic) + 1;
        }
        return num_ifaces;
}

int
collect_implemented_ifaces (MonoClass *klass, MonoClass **ifaces)
{
        int i, count;
        MonoClass *ic;
        
        count = 0;
        for (i = 0; i < klass->interface_count; i++) {
                ic = klass->interfaces [i];
                ifaces[count] = ic;
                count++;
                count += collect_implemented_ifaces(ic, ifaces+count);
        }
        return count;
}

int
get_implemented_interfaces(MonoClass *klass, MonoClass ***ifaces)
{
        int num_ifaces, temp;
        
        num_ifaces = count_implemented_ifaces(klass);
        if (num_ifaces)
        {
                *ifaces = (MonoClass**) malloc(num_ifaces * sizeof(MonoClass*));
                temp = collect_implemented_ifaces(klass, *ifaces);
        }
        return num_ifaces;
}

int
setup_interface_offsets (MonoClass *klass, int cur_slot)
{
        MonoClass *k, *ic;
        int i, max_iid, num_ifaces, temp;
        MonoClass **ifaces;

        /* compute maximum number of slots and maximum interface id */
        max_iid = 0;
        if (klass->parent)
                max_iid = klass->parent->max_interface_id;
        
        num_ifaces = get_implemented_interfaces (klass, &ifaces);
        
        for (i = 0; i < num_ifaces; ++i)
        {
                ic = ifaces[i];
                if (!ic->inited) 
                        init_interface(ic);
                if (max_iid < ic->interface_id)
                        max_iid = ic->interface_id;
        }

        if (max_iid < klass->interface_id)
                max_iid = klass->interface_id;

        klass->max_interface_id = max_iid;
        
        
        /* compute vtable offset for interfaces */
        klass->interface_offsets = malloc (sizeof (int) * (max_iid + 1));

        for (i = 0; i <= max_iid; i++)
                klass->interface_offsets [i] = -1;

        for (i = 0; i < num_ifaces; ++i) 
        {
                ic = ifaces[i];
                klass->interface_offsets [ic->interface_id] = cur_slot;
                cur_slot += ic->method.count;
        }
        if (num_ifaces)
                free(ifaces);
        
        if (klass->parent)
        {
                num_ifaces = get_implemented_interfaces (klass->parent, &ifaces);
                for (i = 0; i < num_ifaces; ++i) 
                {
                        ic = ifaces[i];
                        if (klass->interface_offsets [ic->interface_id] == -1) 
                        {
                                temp = klass->parent->interface_offsets [ic->interface_id];
                                klass->interface_offsets [ic->interface_id] = temp;
                        }
                }
                if (num_ifaces)
                        free(ifaces);
        }

        return cur_slot;
}



void
init_interface(MonoClass* iface)
{
        iface->interface_id = get_next_interface_id();
        iface->inited = 1;
}


void
init_class(MonoClass* klass)
{
        int cur_slot = 0;
        if (klass->parent && !klass->parent->inited)
                init_class (klass->parent);
        
        if (klass->parent) 
                cur_slot = klass->parent->vtable_size;
       
        setup_interface_offsets(klass, cur_slot);

        klass->inited = 1;   
}


void
create_runtime_vtable(MonoClass *klass)
{
        MonoVTable *result;
        int rt_vtable_size, i;
        void **interface_offsets;
    
        rt_vtable_size = sizeof (void*) * (klass->max_interface_id + 1) +
                sizeof (MonoVTable) + sizeof (void*) * klass->vtable_size;
    
        interface_offsets = (void**) malloc(rt_vtable_size);
        result = (MonoVTable*) (interface_offsets + klass->max_interface_id + 1);
    
        memcpy(result, klass->runtime_info->domain_vtables[0], sizeof (MonoVTable) + sizeof (void*) * klass->vtable_size);
        result->max_interface_id = klass->max_interface_id;
        klass->runtime_info->domain_vtables[0] = result;
    
        /* initialize interface offsets */
        for (i = 0; i <= klass->max_interface_id; i++)
        {
                int slot = klass->interface_offsets[i];
                if (slot > -1) 
                        interface_offsets [klass->max_interface_id - i] = (void*) &(result->vtable [slot]);
        }
} 


MonoObject*
ldstr(MonoObject* o)
{
/*  extern void* _ZTVN8mscorlib6System6StringE;
  o->vtable = _ZTVN8mscorlib6System6StringE; */
  return o;
}

void
_ZN8mscorlib6System7Console9WriteLineEPNS0_6StringE(struct MonoString *s)
{
  int i;
  for(i=0;i<s->len;i++)
   putchar(s->chars[i]);
  putchar('\n');
}

void
_ZN8mscorlib6System7Console9WriteLineEl(long l)
{
  printf("%ld\n", l);
}

void
_ZN8mscorlib6System11Environment4ExitEl(long c)
{
  exit(c);
}

/*XXX: this ought to be abstract */
void
_ZN8mscorlib6System5Array6CopyToEPNS1_El()
{}


void
_ZN8mscorlib6System6Object8FinalizeEv (MonoObject *obj)
{
}

int
_ZN8mscorlib6System6Object11GetHashCodeEv (MonoObject *obj)
{
  return 0;
}

int
_ZN8mscorlib6System6Object6EqualsEPNS1_E (MonoObject *obj, MonoObject *obj2)
{
  return obj == obj2;
}

/* TODO: imlement this function */
void*
_ZN8mscorlib6System4Type25internal_type_from_handleENS0_17RuntimeTypeHandleE (void *handle)
{
  return NULL;
}

