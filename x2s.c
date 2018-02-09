/**
 * x2s.c - Convert XML to SML and back
 *
 * Test libxml2 SML parsing and saving ability.
 * Run (x2s -?) to display a help screen.
 *
 * See Copyright for the status of this software.
 *
 * jf.larvoire@free.fr
 */

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlsave.h>

#include <debugm.h> /* SysToolsLib debug macros */

int usage(void);

int usage() {
  printf("%s%s", "\
XML <--> SML converter\n\
\n\
Usage: x2s [OPTIONS] [INPUT_FILENAME [OUTPUT_FILENAME]]\n\
\n\
Options:\n\
  -?            Display this help screen\n\
  -d            Debug mode. Repeat to get more debugging output\n\
  -D            Output no ?xml declaration. Default: Same as in input\n\
  -E            Output no empty tags\n\
  -f            Format and indent the output. Default: Same as the input\n", "\
  -PB           Parse removing blank nodes\n\
  -PE           Parse ignoring errors\n\
  -PN           Parse removing entity nodes (i.e. expanding entities)\n\
  -PW           Parse ignoring warnings\n\
  -s            Output SML. Default if the input is XML\n\
  -S            Output non-significant spaces\n\
  -x            Output XML. Default if the input is SML\n\
\n\
Filenames: Default or \"-\": Use stdin and stdout respectively\n\
\n\
");
  return 0;
}

int main(int argc, char *argv[]) {
  int i;
  xmlDocPtr doc; /* the resulting document tree */
  const char * infilename = NULL;
  const char * outfilename = NULL;
  /* const char * encoding = "ISO-8859-1"; */
  int iSaveOpts = XML_SAVE_AS_SML;
  int iParseOpts = 0;
  int iOutMLTypeSet = 0; /* 1 = Output markup type specified */
  int iXmlDeclSet = 0; /* 1 = Whether to output the ?xml declaration specified */
  xmlSaveCtxtPtr ctxt;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (arg[0] == '-') {
      char *opt = arg + 1;
      if (!strcmp(opt, "?")) {
      	return usage();
      }
      if (!strcmp(opt, "d")) {
	DEBUG_MORE();
      	continue;
      }
      if (!strcmp(opt, "D")) {
      	iXmlDeclSet = 1;
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
      	iSaveOpts |= XML_SAVE_AS_SML;
	iOutMLTypeSet = 1;
      	continue;
      }
      if (!strcmp(opt, "S")) {
      	iSaveOpts |= XML_SAVE_WSNONSIG;
      	continue;
      }
      if (!strcmp(opt, "x")) {
      	iSaveOpts &= ~XML_SAVE_AS_SML;
	iOutMLTypeSet = 1;
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
  DEBUG_PRINTF(("# Debug level %d\n", iDebug));

  if (!infilename) infilename = "-"; /* Input from stdin by default */
  if (!outfilename) outfilename = "-"; /* Output to stdout by default */

  /* Parse the input */
  doc = xmlReadFile(infilename, NULL, iParseOpts);
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse ML in \"%s\"\n", infilename);
    return 1;
  }
  DEBUG_PRINTF(("# Parsed ML successfully\n"));

  /* Output the other ML type if not specified on the command line */
  if (doc->properties & XML_DOC_SML) {		/* The input doc was SML */
    DEBUG_PRINTF(("# The input was SML\n"));
    if (!iOutMLTypeSet) iSaveOpts &= ~XML_SAVE_AS_SML; /* So output XML */
  } else {					/* Else the input doc was XML */
    DEBUG_PRINTF(("# The input was XML\n"));
    if (!iOutMLTypeSet) iSaveOpts |= XML_SAVE_AS_SML;  /* So output SML */
  }

  /* Output an ?xml declaration only if there was one already */
  if (doc->properties & XML_DOC_XMLDECL) {	/* There was one */
    DEBUG_PRINTF(("# The input had an ?xml declaration\n"));
    if (!iXmlDeclSet) iSaveOpts &= ~XML_SAVE_NO_DECL;
  } else {					/* Else there was none */
    DEBUG_PRINTF(("# The input did not have an ?xml declaration\n"));
    if (!iXmlDeclSet) iSaveOpts |= XML_SAVE_NO_DECL;
  }

  /* Generate the output */
  /* xmlKeepBlanksDefault(1); // So that the SML is indented identically  */
  ctxt = xmlSaveToFilename(outfilename, NULL, iSaveOpts);
  xmlSaveDoc(ctxt, doc);
  xmlSaveClose(ctxt);
  xmlFreeDoc(doc);
  xmlCleanupParser(); /* Cleanup function for the XML library. */

  return 0;
}
