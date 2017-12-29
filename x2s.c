/**
 * section: xmlWriter
 * synopsis: use various APIs for the xmlWriter
 * purpose: tests a number of APIs for the xmlWriter, especially
 *          the various methods to write to a filename, to a memory
 *          buffer, to a new document, or to a subtree. It shows how to
 *          do encoding string conversions too. The resulting
 *          documents are then serialized.
 * usage: testWriter
 * test: testWriter && for i in 1 2 3 4 ; do diff $(srcdir)/writer.xml writer$$i.tmp || break ; done
 * author: Alfred Mickautsch
 * copy: see Copyright for the status of this software.
 */
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlsave.h>

#include <debugm.h> /* SysToolsLib debug macros */

#define MY_ENCODING "ISO-8859-1"

void testXmlwriterFilename(const char *uri);
void testXmlwriterMemory(const char *file);
void testXmlwriterDoc(const char *file);
void testXmlwriterTree(const char *file);
xmlChar *ConvertInput(const char *in, const char *encoding);

int usage() {
  printf("%s\n", "\
XML to SML converter\n\
\n\
Usage: x2s [OPTIONS] [XMLFILENAME [SMLFILENAME]]\n\
\n\
Options:\n\
  -d            Debug mode\n\
  -D            No ?xml declaration\n\
  -E            No empty tags\n\
  -f            Format and indent the output. Default: Same as the input\n\
  -PB           Parse removing blank nodes\n\
  -PE           Parse ignoring errors\n\
  -PN           Parse removing entity nodes (i.e. expanding entities)\n\
  -PW           Parse ignoring warnings\n\
  -s            Output non-significant spaces\n\
  -x            Output XML. Default: Output SML\n\
  -?            Display this help screen\n\
\n\
Filenames: Default or \"-\": Use stdin and stdout respectively\n\
\n\
");
  return 0;
}
 
int main(int argc, char *argv[]) {
  int i;
  xmlDocPtr doc; /* the resulting document tree */
  char * infilename = NULL;
  char * outfilename = NULL;
  char * encoding = "ISO-8859-1";
  int iSaveOpts = XML_SAVE_AS_SML;
  int iParseOpts = 0;
  xmlSaveCtxtPtr ctxt;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (arg[0] == '-') {
      char *opt = arg + 1;
      if (!strcmp(opt, "?")) {
      	return usage();
      }
      if (!strcmp(opt, "d")) {
      	DEBUG_ON();
	DEBUG_PRINTF(("Debug is ON\n"));
      	continue;
      }
      if (!strcmp(opt, "D")) {
      	iSaveOpts |= XML_SAVE_NO_DECL;
      	continue;
      }
      if (!strcmp(opt, "E")) {
      	iSaveOpts |= XML_SAVE_NO_EMPTY;
      	continue;
      }
      if (!strcmp(opt, "f")) {
      	iSaveOpts |= XML_SAVE_FORMAT;
      	continue;
      }
      if (!strcmp(opt, "PB")) {
      	iParseOpts |= XML_PARSE_NOBLANKS;
      	continue;
      }
      if (!strcmp(opt, "PE")) {
      	iParseOpts |= XML_PARSE_NOERROR;
      	continue;
      }
      if (!strcmp(opt, "PN")) {
      	iParseOpts |= XML_PARSE_NOENT;
      	continue;
      }
      if (!strcmp(opt, "PW")) {
      	iParseOpts |= XML_PARSE_NOWARNING;
      	continue;
      }
      if (!strcmp(opt, "s")) {
      	iSaveOpts |= XML_SAVE_WSNONSIG;
      	continue;
      }
      if (!strcmp(opt, "x")) {
      	iSaveOpts &= ~XML_SAVE_AS_SML;
      	continue;
      }
      printf("Unexpected option: %s\n", arg);
      exit(1);
    }
    if (!infilename) {
      infilename = arg;
      continue;
    }
    if (!outfilename) {
      outfilename = arg;
      continue;
    }
    printf("Unexpected argument: %s\n", arg);
    exit(1);
  }

  if (!infilename) infilename = "-"; // Input from stdin by default
  if (!outfilename) outfilename = "-"; // Output to stdout by default

  doc = xmlReadFile(infilename, NULL, iParseOpts);
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse ML in \"%s\"\n", infilename);
    return 1;
  }
  DEBUG_PRINTF(("# Parsed ML successfully\n"));
  xmlKeepBlanksDefault(1); // So that the SML is indented identically 
  ctxt = xmlSaveToFilename(outfilename, NULL, iSaveOpts);
  xmlSaveDoc(ctxt, doc);
  xmlSaveClose(ctxt);
  xmlFreeDoc(doc);
  xmlCleanupParser(); // Cleanup function for the XML library.
  return 0;
}

/**
 * ConvertInput:
 * @in: string in a given encoding
 * @encoding: the encoding used
 *
 * Converts @in into UTF-8 for processing with libxml2 APIs
 *
 * Returns the converted UTF-8 string, or NULL in case of error.
 */
xmlChar *
ConvertInput(const char *in, const char *encoding)
{
    xmlChar *out;
    int ret;
    int size;
    int out_size;
    int temp;
    xmlCharEncodingHandlerPtr handler;

    if (in == 0)
        return 0;

    handler = xmlFindCharEncodingHandler(encoding);

    if (!handler) {
        printf("ConvertInput: no encoding handler found for '%s'\n",
               encoding ? encoding : "");
        return 0;
    }

    size = (int) strlen(in) + 1;
    out_size = size * 2 - 1;
    out = (unsigned char *) xmlMalloc((size_t) out_size);

    if (out != 0) {
        temp = size - 1;
        ret = handler->input(out, &out_size, (const xmlChar *) in, &temp);
        if ((ret < 0) || (temp - size + 1)) {
            if (ret < 0) {
                printf("ConvertInput: conversion wasn't successful.\n");
            } else {
                printf
                    ("ConvertInput: conversion wasn't successful. converted: %i octets.\n",
                     temp);
            }

            xmlFree(out);
            out = 0;
        } else {
            out = (unsigned char *) xmlRealloc(out, out_size + 1);
            out[out_size] = 0;  /*null terminating out */
        }
    } else {
        printf("ConvertInput: no mem\n");
    }

    return out;
}
