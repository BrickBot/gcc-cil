#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void*
newobj (void* vtable, size_t size)
{
  MonoObject *obj = (MonoObject*) malloc(size);

  if (obj == 0)
    {
      out_of_memory();
    }

  obj->vtable = vtable;

  obj->synchronisation = 0;

  memset((char*) obj + sizeof(MonoObject), 0, size - sizeof(MonoObject));

  return obj;
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
_ZN8mscorlib6System7Console9WriteLineEb(char b)
{
  printf(b ? "true\n" : "false\n");
}

void
_ZN8mscorlib6System11Environment4ExitEl(long c)
{
  exit(c);
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

