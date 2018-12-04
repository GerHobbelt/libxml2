This is a fork of the...

XML toolkit from the GNOME project
==================================

... adding experimental support for parsing and generating SML.

For information about SML, standing for "Simplified XML", see page 137 of:  
http://archive.xmlprague.cz/2018/files/xmlprague-2018-proceedings.pdf


Changes in the library
----------------------

This version of libxml2 can parse both XML and SML, and save either kind.

By default, it reads and writes XML, and should be perfectly compatible with
the standard libxml2 library.

To use the new SML parsing abilities, use one of the new parser flags:

- XML_PARSE_SML: Parse SML.  
- XML_PARSE_DETECT_ML: Auto-detect the Markup Language type using heuristics.  
  The ML type found is reported in the flag: (doc->properties & XML_DOC_SML).  

To generate SML, use the new save flag:  

- XML_SAVE_AS_SML: Save a DOM tree as SML.

Limitations:  

- The xmlsave functions for SML are implemented, but the xmlwriter's are not.
- The parsing of SML !declarations and ?processing instructions is only
   partially implemented. It'll work only in very simple cases.


New programs
------------

This fork also adds one important program:

- sml2.c: Convert XML to SML, or SML to XML, optionally reformating it.  
  ﻿  
  sml2.c is a reimplemention in C of the sml.tcl SML <--> XML conversion script.
  See: https://github.com/JFLarvoire/SysToolsLib/blob/master/Tcl/sml.tcl  
  sml2.c is a trivial program, doing only the command-line parsing.
  All the hard work is done by the modified libxml2 library.  
  sml2.exe is about 50 times faster than sml.tcl for converting very large files.
  Use option -? or -h to display help.
  I'm particularly proud of the -f option, to reformat and reindent canonically
  the output, whatever its kind.
  This makes sml2.exe particularly useful to study complex XML files.

### Limitation

Contrary to sml.tcl, sml2.c does not guaranty binary reversibility. That is:
An XML file converted to SML, then back to XML, may have changes in its non-
significant white spaces. This is due to a limitation of libxml2, which does
not record non-significant white spaces in the DOM tree.
This is a problem for testing, as it's not possible to just do a binary
comparison to check the sml2.exe conversions correctness.
But this is not a problem for actual use, as non-significant white spaces
are (by definition) non-significant in XML and SML.


Build procedure
---------------

Use the standard build procedure for both Unix and Windows.  
The sml2 or sml2.exe program is built along with the test programs.

For Windows, I've added two batch scripts to help build it with Microsoft tools:  

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



Debug mode
----------

To help debug the SML support in libxml2, I've added lots of debug macros
in the code. These macros are NOOPs in normal builds. They have no effect
on performance.

In debug builds, AND when debug is enabled by the sml2.exe -d option on the
command line, they will output instrumented functions calls and return values.
The calls are indented by call depth, making it easy to follow the program
progress, and see when things start to go wrong.

For details about these macros, see:
https://github.com/JFLarvoire/SysToolsLib/blob/master/C/include/debugm.h
and
https://github.com/JFLarvoire/SysToolsLib/blob/master/Docs/System%20Script%20Libraries%20Description.htm
    
-------------------------------------------------------------------------------

Full documentation for the standard version of libxml2 is available on-line at  

   http://xmlsoft.org/

This code is released under the MIT Licence see the Copyright file.

To build on an Unixised setup:

    ./configure ; make ; make install

To build on Windows:

   see instructions on [win32/Readme.txt](win32/Readme.txt)

To assert build quality:  
   on an Unixised setup:  
      run make tests  
   otherwise:  
       There is 3 standalone tools runtest.c runsuite.c testapi.c, which
       should compile as part of the build or as any application would.
       Launch them from this directory to get results, runtest checks 
       the proper functionning of libxml2 main APIs while testapi does
       a full coverage check. Report failures to the list.

To report bugs, follow the instructions at:  
   http://xmlsoft.org/bugs.html

A mailing-list xml@gnome.org is available, to subscribe:  
   http://mail.gnome.org/mailman/listinfo/xml

The list archive is at:  
   http://mail.gnome.org/archives/xml/

All technical answers asked privately will be automatically answered on
the list and archived for public access unless privacy is explicitly
required and justified.

Daniel Veillard

$Id$
