/*
 * lintmain.c: Main routine for xmllint
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>

#include "private/lint.h"


#ifndef XMLLINT_FUZZ

#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      xml_xmllint_main(cnt, arr)
#endif

int main(int argc, const char** argv) {
    return xmllintMain(argc, argv, stderr, NULL);
}

#endif

