typedef int size_t;
extern void *memory_start;

typedef struct{
  void* vtable;
  void *synchronisation;
}MonoObject;

typedef struct {
        MonoObject obj;
        void *bounds;
        size_t max_length;
        double vector [0];
} MonoArray;

struct MonoString{
  MonoObject obj;
  int len;
  unsigned short chars[];
};


static void *first_free;

static void
init_memory ()
{
  first_free = &memory_start;
}

static void *
memset (void *addr, int s, size_t len)
{
  size_t l;
  int *a;

  a = addr;
  for (l = 0; l < len; l++)
    {
      *(a++) = s;
    }
  
  return addr;
}

static void *
malloc (size_t size)
{ 
  void * p;

  p = first_free;
  first_free += size;

  return p;
}

void*
newobj (void* vtable, size_t size)
{
  MonoObject *obj;

  obj = (MonoObject *) malloc (size);

  if (obj == 0)
    {
      /* TODO: generate OutOfMemoryException */
    }

  obj->vtable = vtable;
  obj->synchronisation = 0;

  memset ((char *) obj + sizeof (MonoObject), 0, size - sizeof (MonoObject));

  return obj;
}

void*
newarr (size_t length, size_t elem_size)
{
  MonoArray *array;

  /* TODO: provide vtable */
  array = (MonoArray *) newobj (0, length * elem_size + sizeof (MonoArray));

  if (array == 0)
    {
      /* TODO: generate OutOfMemoryException */
    }

  array->bounds = 0;
  array->max_length = length;

  return array;
}


void
_ZN8mscorlib6System6Object8FinalizeEv (MonoObject *obj)
{
}

int
_ZN8mscorlib6System6Object11GetHashCodeEv (MonoObject *obj)
{
  return 0;
}

struct MonoString *
_ZN8mscorlib6System6Object8ToStringEv (MonoObject *obj)
{
  return 0;
}

/*XXX: this ought to be abstract */
void
_ZN8mscorlib6System5Array6CopyToEPNS1_El()
{}

void
init_runtime ()
{
  init_memory ();
}
