The following generated files shall BE included in the subversion repository:

* config-lang.in generated from config-lang.in.in.
* Make-lang.in generated from Make-lang.in.in and all .d files in the 
  subdirectories


The following generated files shall NOT be included in the subversion 
repository:

* *.d in all subdirectories. These are Makefile fragments containing 
  dependencies for the respective .c source file. Not to be included since 
  their content is put into Make-lang.in.

For generating config-lang.in, Make-lang.in and the .d files, go to your binary
directory where you have built the GCC before (if you haven't build the GCC 
before, you need to do so in order to make the script determine dependencies 
correctly due to generated header files). Your binary directory should not be a
subdirectory of the source directory.

In the binary directory, the GCC build process created a directory gcc which 
you need to switch to. From there, you have to call the Makefile of the scil 
source directory to generate the files.

Never edit Make-lang.in or config-lang.in directly but only their %.in.in 
source. You could run Makefile whenever you made changes to any files but you
only need to in the following cases:

Make-lang.in has to be redone whenever you

* add or remove .c files
* add or remove references to header files to or from .c files
* add the first GTY tag to or remove the last GTY tag from a .c file
* make changes to Make-lang.in.in

config-lang.in has to be redone whenever you 

* add the first GTY tag to or remove the last GTY tag from a .c or .h file
* make changes to config-lan.in.in

If config-lang.in was redone, you have to call GCC's configure script from the
binary directory.
