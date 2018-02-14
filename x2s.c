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

#define VERSION "2018-02-14"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlsave.h>

#include <debugm.h> /* SysToolsLib debug macros */

/* Forward references */
int usage(void);
int xmlRemoveBlankNodes(xmlNodePtr n);
int xmlTrimTextNodes(xmlNodePtr n);

int usage() {
  printf("%s%s", "\
XML <--> SML converter\n\
\n\
Usage: x2s [OPTIONS] [INPUT_FILENAME [OUTPUT_FILENAME]]\n\
\n\
Options:\n\
  -?            Display this help screen\n\
  -B            Delete blank nodes\n\
"
DEBUG_CODE("\
  -d            Debug mode. Repeat to get more debugging output\n\
"
)
"\
  -D            Output no ?xml declaration. Default: Same as in input\n\
  -E            Output no empty tags\n\
  -f            Format and indent the output. Default: Same as the input\n", "\
  -PB           Parse removing blank nodes\n\
  -PE           Parse ignoring errors\n\
  -PN           Parse removing entity nodes (i.e. expanding entities)\n\
  -PW           Parse ignoring warnings\n\
  -s            Output SML. Default if the input is XML\n\
  -S            Output non-significant spaces\n\
  -T            Trim text nodes\n\
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
  int iDeleteBlankNodes = 0;
  int iTrimSpaces = 0;

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (arg[0] == '-') {
      char *opt = arg + 1;
      if (!strcmp(opt, "?")) {
      	return usage();
      }
      if (!strcmp(opt, "B")) {
      	iDeleteBlankNodes = 1;
      	continue;
      }
      DEBUG_CODE(
      if (!strcmp(opt, "d")) {
	DEBUG_MORE();
      	continue;
      }
      )
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
      	/* iParseOpts |= XML_PARSE_NOBLANKS; /* This does not seem to have any effect */
      	iDeleteBlankNodes = 1; /* Do it manually instead */
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
      if (!strcmp(opt, "t")) {
      	iTrimSpaces = 1;
      	continue;
      }
      if ((!strcmp(opt, "V")) || (!strcmp(opt, "-version"))) {
      	printf("x2s version " VERSION " " EXE_OS_NAME);
      	DEBUG_CODE(
      	printf(" Debug");
      	)
        printf("\n");
      	exit(0);
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

#if 0  /* This does not seem to have any effect */
  if (iSaveOpts & XML_SAVE_FORMAT) { /* If we want to reformat the output */
    i = xmlKeepBlanksDefault(0); /* Then ignore blank nodes in input */
    DEBUG_PRINTF(("xmlKeepBlanksDefault was %d\n", i)); // Prints a 1 */
  }
#endif

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

  if (iDeleteBlankNodes) { /* If we want to reformat the output */
    xmlRemoveBlankNodes(xmlDocGetRootElement(doc)); // Remove blank text nodes
  }

  if (iTrimSpaces) { /* If we want to reformat the output */
    xmlTrimTextNodes(xmlDocGetRootElement(doc)); // Trim spaces around text nodes
  }

  /* Generate the output */
  ctxt = xmlSaveToFilename(outfilename, NULL, iSaveOpts);
  xmlSaveDoc(ctxt, doc);
  xmlSaveClose(ctxt);
  xmlFreeDoc(doc);
  xmlCleanupParser(); /* Cleanup function for the XML library. */

  return 0;
}

/**
 * Remove blank text nodes in a DOM node tree
 */
                                                                         
int xmlRemoveBlankNodes(xmlNodePtr node) {
  xmlNodePtr child;

  if (node) {
    if (xmlIsBlankNode(node)) {
      DEBUG_CODE(
      xmlChar *pszText = xmlNodeGetContent(node);
      DEBUG_PRINTF(("xmlUnlinkNode(%s)\n", dbgStr(pszText)));
      xmlFree(pszText);
      )
      xmlUnlinkNode(node);
      xmlFreeNode(node);
    } else {
      xmlNodePtr next;
      for (child = node->children; child; child = next) {
      	next = child->next; // Do that before deleting the node!
	xmlRemoveBlankNodes(child);
      }
    }
  }

  return 0;
}

/**
 * Trim spaces around text in a DOM node tree
 *
 * Danger: This may remove significant spaces, in strings that really do have
 *         head or tail spaces
 */
                                                                         
int xmlTrimTextNodes(xmlNodePtr node) {
  xmlNodePtr child;

  if (node) {
    if (node->type == XML_TEXT_NODE) {
      xmlChar *pData = xmlNodeGetContent(node);
      xmlChar *pData0 = pData;
      xmlChar *pc;
      int changed = 0;
      /* Trim the left side */
      for (; *pData && isspace(*pData); pData++) changed = 1;
      /* Trim the right side */
      for (pc = pData+strlen(pData); (pc > pData) && isspace(*(pc-1)); ) {
      	*(--pc) = '\0';
      	changed = 1;
      }
      if (changed) xmlNodeSetContent(node, pData);
      xmlFree(pData0);
    } else {
      for (child = node->children; child; child = child->next) {
	xmlTrimTextNodes(child);
      }
    }
  }

  return 0;
}
