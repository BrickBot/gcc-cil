#include <stdio.h>

typedef struct
{
  void* vtable;
  void *synchronisation;
} MonoObject;

typedef struct 
{
  MonoObject obj;
  void *bounds;
  size_t max_length;
  double vector [0];
} MonoArray;

typedef struct 
{
  MonoObject obj;
  int len;
  unsigned short chars[];
} MonoString;


void
WriteLine (MonoString *str)
{
  int i;
  for(i = 0; i < str->len; i++)
    {
      putchar (str->chars[i]);
    }
  putchar ('\n');
}

void
Exit (int code)
{
  exit (code);
}
