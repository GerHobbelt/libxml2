
                             Windows port
                             ============

This directory contains the files required to build this software on the
native Windows platform. This is not a place to look for help if you are
using a POSIX emulator, such as Cygwin. Check the Unix instructions for 
that.

Changes for SML support
-----------------------

These changes are actually minor improvements, to simplify the Windows build process. 

I've added two batch scripts to help build it with Microsoft tools:  

- configure.bat: Front end to the existing configure.js script.  
- make.bat: Front end to Microsoft nmake.exe or Borland bmake.exe.
  (Copied by configure.js from make.msvc.bat and make.bcb.bat respectively.)

These scripts allow building the Windows version with commands that look the 
same as the typical commands used to build the Unix versions:

To quickly build libxml2.lib and sml2.exe in Windows with Microsoft tools:  
(Assuming you don't have the iconv library installed, as it is not by default.)  

- Run the vcvars*.bat for your Visual C++ version.

      :# Locate Visual Studio installation path in vswhere output:
      vswhere -latest | findstr installationPath:
      :# Locate the vcvars*.bat scripts in Visual Studio installation. Ex:
      dir /b /s "C:\Program Files (x86)\Microsoft Visual Studio\2017\vcvars*.bat"
      :# Open a new sub-shell, so that environment changes can be reversed.
      cmd
      :# Run vcvars.bat for your target processor. Ex:
      "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

- Go to the libxml2 win32 subdirectory.

      cd win32

- Run: 

      configure iconv=no xml_debug=no
      make STATIC=1 libxmla bin.msvc\sml2.exe

- sml2.exe is stored in win32\bin.msvc

To build libxml2.lib and sml2.exe debug versions in Windows with Microsoft tools:

    make clean
    configure iconv=no xml_debug=yes
    make DEBUG=1 STATIC=1 libxmla bin.msvc\sml2.exe  

To build the version for another processor, for example the 32-bits x86 processor:

    :# Exit the current sub-shell to return to a "clean" environment, and open a new one.
    exit
    cmd
    :# Run vcvars.bat for the other processor. Ex:
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
    make clean
    make STATIC=1 libxmla bin.msvc\sml2.exe


-----------------------------------------------------------------------------------------------------------------------

Original Readme.txt contents:

CONTENTS
--------

1. General  
   1.1 Building From the Command-Line  
   1.2 Configuring The Source  
   1.3 Compiling  
   1.4 Installing  

2. Compiler Specifics  
   2.1 Microsoft Visual C/C++  
   2.1 GNU C/C++, Mingw Edition  
   2.2 Borland C++ Builder  
      2.2.1 Building with iconv support  
      2.2.2 Compatability problems with MSVC (and probably CYGWIN)  
      2.2.3 Other caveats  




## 1. General


### 1.1 Building From The Command-Line

This is the easiest, preferred and currently supported method. It can
be that a subdirectory of the directory where this file resides 
contains project files for some IDE. If you want to use that, please
refer to the readme file within that subdirectory.

In order to build from the command-line you need to make sure that
your compiler works from the command line. This is not always the
case, often the required environment variables are missing. If you are
not sure, test if this works first. If it doesn't, you will first have
to configure your compiler suite to run from the command-line - please
refer to your compiler's documentation regarding that.

The first thing you want to do is configure the source. You can have
the configuration script do this automatically for you. The
configuration script is written in JScript, a Microsoft's
implementation of the ECMA scripting language. Almost every Windows
machine can execute this through the Windows Scripting Host. If your
system lacks the ability to execute JScript for some reason, you must
perform the configuration manually and you are on your own with that.

The second step is compiling the source and, optionally, installing it
to the location of your choosing.


### 1.2 Configuring The Source

The configuration script accepts numerous options. Some of these
affect features which will be available in the compiled software,
others affect the way the software is built and installed. To see a
full list of options supported by the configuration script, run

    cscript configure.js help

from the win32 subdirectory. The configuration script will present you
the options it accepts and give a biref explanation of these. In every
case you will have two sets of options. The first set is specific to
the software you are building and the second one is specific to the
Windows port.

Once you have decided which options suit you, run the script with that
options. Here is an example:

    cscript configure.js compiler=msvc prefix=c:\opt include=c:\opt\include lib=c:\opt\lib debug=yes

The previous example will configure the process to use the Microsoft's
compiler, install the library in c:\opt, use c:\opt\include and 
c:\opt\lib as additional search paths for the compiler and the linker 
and build executables with debug symbols.

Note: Please do not use path names which contain spaces. This will
fail. Allowing this would require me to put almost everything in the
Makefile in quotas and that looks quite ugly with my
syntax-highlighting engine. If you absolutely must use spaces in paths
send me an email and tell me why. If there are enough of you out there
who need this, or if a single one has a very good reason, I will
modify the Makefile to allow spaces in paths.


### 1.3 Compiling

After the configuration stage has been completed, you want to build
the software. You will have to use the make tool which comes with
your compiler. If you, for example, configured the source to build
with Microsoft's MSVC compiler, you would use the NMAKE utility. If
you configured it to build with GNU C compiler, mingw edition, you
would use the GNU make. Assuming you use MSVC, type

    nmake /f Makefile.msvc

and if you use MinGW, you would type

    make -f Makefile.mingw

and if you use Borland's compiler, you would type

    bmake -f Makefile.bcb

in the win32 subdirectory. When the building completes, you will find
the executable files in win32\bin.* directory, where * stands for the
name of the compiler you have used.


### 1.4 Installing

You can install the software into the directory you specified to the
configure script during the configure stage by typing (with MSVC in
this example)

    nmake /f Makefile.msvc install

That would be it, enjoy.





## 2. Compiler Specifics


### 2.1 Microsoft Visual C/C++

If you use the compiler which comes with Visual Studio .NET, note that
it will link to its own C-runtime named msvcr70.dll or msvcr71.dll. This 
file is not available on any machine which doesn't have Visual Studio 
.NET installed.


### 2.2 GNU C/C++, Mingw edition

When specifying paths to configure.js, please use slashes instead of 
backslashes for directory separation. Sometimes Mingw needs this. If
this is the case, and you specify backslashes, then the compiler will 
complain about not finding necessary header files.


### 2.2 Borland C++ Builder

To compile libxml2 with the BCB6 compiler and associated tools, just follow
the basic instructions found in this file file. Be sure to specify 
the "compiler=bcb" option when running the configure script. To compile the
library and test programs, just type

    make -fMakefile.bcb

That should be all that's required. But there are a few other things to note:

#### 2.2.1 Building with iconv support

If you configure libxml2 to include iconv support, you will obviously need to
obtain the iconv library and include files. To get them, just follow the links 
at http://www.gnu.org/software/libiconv/ - there are pre-compiled Win32 
versions available, but note that these where built with MSVC. Hence the 
supplied import library is in COFF format rather than OMF format. You can 
convert this library by using Borland's COFF2OMF utility, or use IMPLIB to 
build a new import library from the DLL. Alternatively, it is possible to
obtain the iconv source, and build the DLL using the Borland compiler.

There is a minor problem with the header files for iconv - they expect a
macro named "EILSEQ" in errno.h, but this is not defined in the Borland
headers, and its absence can cause problems. To circumvent this problem, I
define EILSEQ=2 in Makefile.bcb. The value "2" is the value for ENOFILE (file
not found). This should not have any disastrous side effects beyond possibly
displaying a misleading error message in certain situations.

#### 2.2.2 Compatability problems with MSVC (and probably CYGWIN)

A libxml2 DLL generated by BCB is callable from MSVC programs, but there is a
minor problem with the names of the symbols exported from the library. The
Borland compiler, by default, prepends an underscore character to global 
identifiers (functions and global variables) when generating object files.
Hence the function "xmlAddChild" is added to the DLL with the name
"_xmlAddChild". The MSVC compiler does not have this behaviour, and looks for
the unadorned name. I currently circumvent this problem by writing a .def file
which causes BOTH the adorned and unadorned names to be exported from the DLL.
This behaviour may not be supported in the future.

An even worse problem is that of generating an import library for the DLL. The
Borland-generated DLL is in OMF format. MSVC expects libraries in COFF format,
but they don't provide a "OMF2COFF" utility, or even the equivalent of
Borland's IMPLIB utility. But it is possible to create an import lib from the
.def file, using the command:

    LIB /DEF:libxml2.def

If you don't have the .def file, it's possible to create one manually. Use
DUMPBIN /EXPORTS /OUT:libxml2.tmp libxml2.dll to get a list of the exported
names, and edit this into .def file format.

A similar problem is likely with Cygwin.

#### 2.2.3 Other caveats

We have tested this only with BCB6, Professional Edition, and BCB 5.5 free
command-line tools.



Authors: Igor Zlatkovic <igor@zlatkovic.com>
         Eric Zurcher <Eric.Zurcher@csiro.au>
         Jean-François Larvoire <jf.larvoire@free.fr>
