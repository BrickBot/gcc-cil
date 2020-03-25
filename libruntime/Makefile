###################################
## makefile to create libruntime ##
###################################


TOOLPREFIX=/cygdrive/z/devel/RtNet/gcc/bin/


GCC=$(TOOLPREFIX)gcc

COPT=-O2 -fno-builtin -fomit-frame-pointer
CINC=
CFLAGS=$(COPT) $(CINC)

libruntime.o: libruntime.c libruntime.h
	gcc $*.c -c $(CFLAGS)


