#ifndef __LIBXML_WIN32_CONFIG__
#define __LIBXML_WIN32_CONFIG__

/* Avoid silly warnings about "insecure" functions. */
#define _CRT_SECURE_NO_DEPRECATE 1
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS  1
#endif
#define _CRT_NONSTDC_NO_WARNINGS 1


#define HAVE_SYS_STAT_H
#define HAVE_STAT
#define HAVE_FCNTL_H
#include <winsock2.h> /* struct timeval */
#include <windows.h>
#include <io.h>
#include <direct.h>

#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER >= 1600)
  #define HAVE_STDINT_H
#endif

#if defined(_MSC_VER)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #if _MSC_VER < 1500
    #define vsnprintf(b,c,f,a) _vsnprintf(b,c,f,a)
  #endif
#endif

#endif /* __LIBXML_WIN32_CONFIG__ */

