# Makefile to prepare the front end's Make-lang.in and config-lang.in
#
# Must only be called from the gcc subdirectory in the binary directory!
# This must be outside the source directory.
#
# It is not intended to make it into a release but to ease the burden of
# having to manage the dependencies by hand. Therefore it can be quite 
# specific to GNU make.
#
# Should be invoked after
#  * adding or removing files
#  * adding or removing garbage collection support in one or more files
#  * changes to config-lang.in.in or Make-lang.in.in
#
# You probably have to adjust the $(srcdir) variable below which points
# to the gcc source directory.


srcdir:=../../trunk/gcc

DIRS := $(srcdir)/scil $(srcdir)/scil/pnet/image $(srcdir)/scil/pnet/include $(srcdir)/scil/pnet/support
INCLUDE_DIRS := $(DIRS) $(srcdir)/ $(srcdir)/../include $(srcdir)/../libcpp/include .

HEADERS := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.h))
SOURCES := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))
MAKEFILES := $(SOURCES:%.c=%.d)
BRACKETS :=((

GTY_HEADERS := $(shell grep -l "GTY *$(value BRACKETS)" $(HEADERS))
GTY_SOURCES := $(shell grep -l "GTY *$(value BRACKETS)" $(SOURCES))
GTY_GENERATED := gtype-scil.h $(GTY_SOURCES:$(srcdir)/scil/%.c=gt-scil-%.h)


.phony : all clean

all: $(srcdir)/scil/config-lang.in $(srcdir)/scil/Make-lang.in

$(srcdir)/scil/Make-lang.in : $(srcdir)/scil/Make-lang.in.in $(MAKEFILES) $(HEADERS)
	sed \
	-e 's,SCIL_OBJECTS *=,SCIL_OBJECTS = $(SOURCES:$(srcdir)/%.c=%.o),' \
	-e 's,SCIL_INCLUDES *=,SCIL_INCLUDES = $(addprefix -I,$(subst $(srcdir)/,$$(srcdir)/,$(DIRS))),' \
	$(srcdir)/scil/Make-lang.in.in > $(srcdir)/scil/Make-lang.in
	echo SCIL_GTY_GENERATED = $(GTY_GENERATED) >> $(srcdir)/scil/Make-lang.in
	echo >> $(srcdir)/scil/Make-lang.in
	cat $(MAKEFILES) >> $(srcdir)/scil/Make-lang.in

$(srcdir)/scil/config-lang.in : $(srcdir)/scil/config-lang.in.in $(SOURCES) $(HEADERS)
	cp -f $(srcdir)/scil/config-lang.in.in $(srcdir)/scil/config-lang.in
	echo gtfiles='"$(strip $(subst $(srcdir)/,\$$(srcdir)/,$(GTY_SOURCES) $(GTY_HEADERS)))"' >> $(srcdir)/scil/config-lang.in
	echo >> $(srcdir)/scil/config-lang.in
       
%.d : %.c
	$(CC) $(CPPFLAGS) $(addprefix -I,$(INCLUDE_DIRS)) -MM -MT $(subst $(srcdir)/,,$*.o) $< \
	| sed -e 's,\($(subst $(srcdir)/,,$*)\)\.o[ :]*,\1.o $@ : ,g' -e 's,$(srcdir),$$(srcdir),g' > $@
	@echo >> $@


include $(MAKEFILES)

clean:
	rm -f $(MAKEFILES)
