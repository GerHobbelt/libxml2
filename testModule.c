/*
 * testModule.c : a small tester program for xmlModule
 *
 * See Copyright for the status of this software.
 *
 * joelwreed@comcast.net
 */

#define XML_DEPRECATED

#include "libxml.h"
#include <stdio.h>
#include <libxml/xmlversion.h>

#include <libxml/monolithic_examples.h>

#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      xml_testmodule_main(cnt, arr)
#endif

#ifdef LIBXML_MODULES_ENABLED

#include <limits.h>
#include <string.h>
#include <stdarg.h>

#include <libxml/xmlmemory.h>
#include <libxml/xmlmodule.h>
#include <libxml/xmlstring.h>

#ifdef _WIN32
#define MODULE_PATH "."
#include <stdlib.h> /* for _MAX_PATH */
#ifndef __MINGW32__
#define PATH_MAX _MAX_PATH
#endif
#else
#define MODULE_PATH ".libs"
#endif

/* Used for SCO Openserver*/
#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#else
#define PATH_MAX 1024
#endif
#endif

typedef int (*hello_world_t)(void);

int main(int argc, const char** argv) {
    xmlChar filename[PATH_MAX];
    xmlModulePtr module = NULL;
    hello_world_t hello_world = NULL;

    /* build the module filename, and confirm the module exists */
    xmlStrPrintf(filename, sizeof(filename),
                 "%s/testdso%s",
                 (const xmlChar*)MODULE_PATH,
                 (const xmlChar*)LIBXML_MODULE_EXTENSION);

    module = xmlModuleOpen((const char*)filename, 0);
    if (module == NULL) {
      fprintf(stderr, "Failed to open module\n");
      return(1);
    }

    if (xmlModuleSymbol(module, "hello_world", (void **) &hello_world)) {
      fprintf(stderr, "Failure to lookup\n");
      return(1);
    }
    if (hello_world == NULL) {
      fprintf(stderr, "Lookup returned NULL\n");
      return(1);
    }

    (*hello_world)();

    xmlModuleClose(module);

    return(0);
}

#else

int main(int argc, const char** argv) {
    printf("%s : Module support not compiled in\n", argv[0]);
    return(0);
}
#endif /* LIBXML_MODULES_ENABLED */
