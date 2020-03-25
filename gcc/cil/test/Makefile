ifneq (,$(wildcard /usr/bin/*.exe))
exeext=.exe
else
exeext=
endif

CFLAGS=-g

TESTFILES=${wildcard test_*.cs}

all:	${TESTFILES:%.cs=%.exe} ${TESTFILES:%.cs=%.s} ${TESTFILES:%.cs=%-run}

mscorlib.dll:	mscorlib.cs
	./cscompile.sh -t:library -nostdlib mscorlib.cs

%.s:	%.exe mscorlib.dll
	./cil1 -quiet $< || (rm $@; exit 1)

%.s:	%.dll
	./cil1 -quiet $<

test_%.exe:	test_%.il
	./ilcompile.sh $<

test_%.exe:	test_%.cs
	./cscompile.sh $<

.PRECIOUS:	mscorlib.o runtime.o

%-target$(exeext):	%.s mscorlib.o runtime.o
	gcc -o $@ $^

%-run:	%-target${exeext}
	./$<

clean:
	rm -f *.s *.o *.exe *-target* .gdb_history
