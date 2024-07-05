# gcc-cil
An "official" gcc branch that adds CIL capabilities to gcc, for use with
[brickOS-bibo](https://github.com/BrickBot/brickOS-bibo) and [Lego.NET](https://github.com/BrickBot/gcc-cil)

## Usage
Used to build projects such as [brickOS-bibo](https://github.com/BrickBot/brickOS-bibo),
this particular gcc branch with CIL is also used by the Lego.NET project.

In addition to cross-compiling C and C++ code to the Renesas H8/300 processor
(the processor in the Lego MindStorms RCX),
the CIL translates the Microsoft/ECMA intermediate language of a .NET binary
into native machine code of the target processor.


## Background
Based on a review of the [GCC SVN repository archive](https://gcc.gnu.org/git/?p=gcc-old.git;a=heads),
the "st" branch no longer appears to exist.  The starting point for the code in this
repository is the version of GCC 4.1 that was used in the creation of the Lego.NET v1.4 release.


## Additional Information
More information regarding CIL is available at the following links
* [Contextual Overview](https://www.mono-project.com/archived/gcc4cil/), though not part of the GNU project
* [GNU GCC CLI Back-End and Front-End](https://gcc.gnu.org/projects/cli.html)
* 2006 Google Summer of Code
  - [Project Description](https://www.mono-project.com/archived/summer2006/#gcc-cil-backend)
  - [Project Blog](https://gcc-cil.blogspot.com)


The remainder of this file is the contents of the original GCC README file

* * *

This directory contains the GNU Compiler Collection (GCC).

The GNU Compiler Collection is free software.  See the file COPYING
for copying permission.  The manuals, and some of the runtime
libraries, are under different terms; see the individual source files
for details.

The directory INSTALL contains copies of the installation information
as HTML and plain text.  The source of this information is
gcc/doc/install.texi.  The installation information includes details
of what is included in the GCC sources and what files GCC installs.

See the file gcc/doc/gcc.texi (together with other files that it
includes) for usage and porting information.  An online readable
version of the manual is in the files gcc/doc/gcc.info*.

See http://gcc.gnu.org/bugs.html for how to report bugs usefully.
