#include "libruntime.h"
void* malloc(size_t);
void* memset(void *, int, size_t);
void* memcpy(void *, const void *, size_t);

static int
collect_implemented_ifaces (MonoClass *klass, MonoClass **ifaces);
static int
count_implemented_ifaces (MonoClass *klass);
static void 
create_runtime_vtable(MonoClass *klass);
static int
get_implemented_interfaces(MonoClass *klass, MonoClass ***ifaces);
static short
get_next_interface_id(void);
static void
init_class(MonoClass *klass);
static void
init_interface(MonoClass *klass);
static int
setup_interface_offsets (MonoClass *klass, int cur_slot);

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

void*
newarr (size_t length, size_t elem_size)
{
  MonoArray *array = (MonoArray*) malloc(length * elem_size + sizeof(MonoArray));
  
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

void *
ldstr (MonoString *str)
{
	extern MonoVTable _ZN8mscorlib6System6StringC1Ev;
	str->obj.vtable = &_ZN8mscorlib6System6StringC1Ev;
	return str;
}

size_t
_ZN8mscorlib6System6String10get_LengthEv(MonoString *str)
{
	return str->length;
}

/*
 * The following functions set up a virtual method table
 */

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
  klass->interface_offsets = (int *) malloc (sizeof (int) * (max_iid + 1));
  
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

short
get_next_interface_id(void)
{
  static short id = 0;
  return id++;
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
