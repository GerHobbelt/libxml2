cmake_minimum_required(VERSION 3.2)
project(libxml2 VERSION 2.9.7)

add_library(
    libxml2
    HTMLparser.c
    HTMLtree.c
    SAX.c
    SAX2.c
    buf.c
    c14n.c
    catalog.c
    chvalid.c
    debugXML.c
    dict.c
    encoding.c
    entities.c
    error.c
    globals.c
    hash.c
    legacy.c
    list.c
    nanoftp.c
    nanohttp.c
    parser.c
    parserInternals.c
    pattern.c
    relaxng.c
    schematron.c
    threads.c
    tree.c
    uri.c
    valid.c
    xinclude.c
    xlink.c
    xmlIO.c
    xmlmemory.c
    xmlmodule.c
    xmlreader.c
    xmlregexp.c
    xmlsave.c
    xmlschemas.c
    xmlschemastypes.c
    xmlstring.c
    xmlunicode.c
    xmlwriter.c
    xpath.c
    xpointer.c
    xzlib.c
)

target_compile_definitions(libxml2 PRIVATE _REENTRANT HAVE_CONFIG_H)

include(CheckLibraryExists)
check_library_exists(m "pow" "" __math_system_library)
if(__math_system_library)
  target_link_libraries(libxml2 PUBLIC m)
endif()

target_include_directories(
    libxml2 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

find_package(Threads REQUIRED)
target_link_libraries(libxml2 PUBLIC Threads::Threads)

include(GNUInstallDirs)

set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(NAMESPACE "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "cmake/template/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

install(
    DIRECTORY "include/libxml"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libxml2"
    FILES_MATCHING
    PATTERN "*.h"
)

install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${NAMESPACE}"
    DESTINATION "${config_install_dir}"
)

install(
    TARGETS libxml2
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libxml2"
)

# xmlversion.h {

set(VERSION ${PROJECT_VERSION})

set(a ${PROJECT_VERSION_MAJOR})
set(b ${PROJECT_VERSION_MINOR})
set(c ${PROJECT_VERSION_PATCH})
math(EXPR LIBXML_VERSION_NUMBER "${a} * 10000 + ${b} * 100 + ${c}")

set(LIBXML_VERSION_EXTRA "")

set(WITH_C14N 1)
set(WITH_CATALOG 1)
set(WITH_DEBUG 1)
set(WITH_DOCB 1)
set(WITH_FTP 1)
set(WITH_HTML 1)
set(WITH_HTTP 1)
set(WITH_ISO8859X 1)
set(WITH_LEGACY 1)
set(WITH_MEM_DEBUG 0)
set(WITH_MODULES 1)
set(WITH_OUTPUT 1)
set(WITH_PATTERN 1)
set(WITH_PUSH 1)
set(WITH_READER 1)
set(WITH_REGEXPS 1)
set(WITH_RUN_DEBUG 0)
set(WITH_SAX1 1)
set(WITH_SCHEMAS 1)
set(WITH_SCHEMATRON 1)
set(WITH_THREADS 1)
set(WITH_THREAD_ALLOC 0)
set(WITH_TREE 1)
set(WITH_TRIO 0)
set(WITH_VALID 1)
set(WITH_WRITER 1)
set(WITH_XINCLUDE 1)
set(WITH_XPATH 1)
set(WITH_XPTR 1)

set(MODULE_EXTENSION ${CMAKE_SHARED_MODULE_SUFFIX})

set(WITH_ICU 0)
set(WITH_ZLIB 0)
set(WITH_LZMA 0)
set(WITH_ICONV 0)

set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/generated)

configure_file(
    include/libxml/xmlversion.h.in
    ${gen_dir}/libxml/xmlversion.h
    @ONLY
)

install(
    FILES ${gen_dir}/libxml/xmlversion.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libxml2/libxml"
)

target_include_directories(libxml2 PUBLIC $<BUILD_INTERFACE:${gen_dir}>)

# }

# config.h {

set(bindir ${CMAKE_CURRENT_BINARY_DIR}/try_compile)

try_compile(
    GETHOSTBYNAME_CONST_CHAR
    ${bindir}
    ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/GETHOSTBYNAME_CONST_CHAR.c
)

if(GETHOSTBYNAME_CONST_CHAR)
  set(GETHOSTBYNAME_ARG_CAST " ")
else()
  set(GETHOSTBYNAME_ARG_CAST "(char*)")
endif()

try_compile(
    HAVE_VA_COPY
    ${bindir}
    ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/HAVE_VA_COPY.c
)

if (NOT HAVE_VA_COPY)
  try_compile(
      HAVE___VA_COPY
      ${bindir}
      ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/HAVE___VA_COPY.c
  )
endif()

try_compile(
    VA_LIST_IS_NOT_ARRAY
    ${bindir}
    ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/VA_LIST_IS_NOT_ARRAY.c
)

if(VA_LIST_IS_NOT_ARRAY)
  set(VA_LIST_IS_ARRAY FALSE)
else()
  set(VA_LIST_IS_ARRAY TRUE)
endif()

try_compile(
    XML_SOCKLEN_T_IS_SOCKLEN_T
    ${bindir}
    ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/XML_SOCKLEN_T_IS_SOCKLEN_T.c
)

if(XML_SOCKLEN_T_IS_SOCKLEN_T)
  set(XML_SOCKLEN_T socklen_t)
else()
  try_compile(
      XML_SOCKLEN_T_IS_SIZE_T
      ${bindir}
      ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/XML_SOCKLEN_T_IS_SIZE_T.c
  )
  if(XML_SOCKLEN_T_IS_SIZE_T)
    set(XML_SOCKLEN_T size_t)
  else()
    try_compile(
        XML_SOCKLEN_T_IS_INT
        ${bindir}
        ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/XML_SOCKLEN_T_IS_INT.c
    )
    if(XML_SOCKLEN_T_IS_INT)
      set(XML_SOCKLEN_T int)
    else()
      set(XML_SOCKLEN_T "unsigned int")
    endif()
  endif()
endif()

try_compile(
    XML_SEND_CONST_CHAR
    ${bindir}
    ${CMAKE_CURRENT_LIST_DIR}/cmake/try_compile/XML_SEND_CONST_CHAR.c
)
if(XML_SEND_CONST_CHAR)
  set(SEND_ARG2_CAST " ")
else()
  set(SEND_ARG2_CAST "(char*)")
endif()

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)

check_include_file(ansidecl.h HAVE_ANSIDECL_H)
check_include_file(ctype.h HAVE_CTYPE_H)
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(errno.h HAVE_ERRNO_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(float.h HAVE_FLOAT_H)
check_include_file(fp_class.h HAVE_FP_CLASS_H)
check_include_file(ieeefp.h HAVE_IEEEFP_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(math.h HAVE_MATH_H)
check_include_file(nan.h HAVE_NAN_H)
check_include_file(netdb.h HAVE_NETDB_H)
check_include_file(poll.h HAVE_POLL_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdarg.h HAVE_STDARG_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(time.h HAVE_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(dl.h HAVE_DL_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(signal.h HAVE_SIGNAL_H)

if(HAVE_SYS_TYPES_H)
  set(sys_types "sys/types.h")
else()
  set(sys_types "")
endif()

check_include_files("${sys_types};arpa/inet.h" HAVE_ARPA_INET_H)
check_include_files("${sys_types};arpa/nameser.h" HAVE_ARPA_NAMESER_H)
check_include_files("${sys_types};netinet/in.h" HAVE_NETINET_IN_H)

set(headers "")
if(HAVE_NETINET_IN_H)
  list(APPEND headers "netinet/in.h")
endif()
if(HAVE_ARPA_NAMESER_H)
  list(APPEND headers "arpa/nameser.h")
endif()
list(APPEND headers "resolv.h")
check_include_files("${headers}" HAVE_RESOLV_H)

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
check_function_exists(dlopen HAVE_DLOPEN)
set(CMAKE_REQUIRED_LIBRARIES "")

check_function_exists(_stat HAVE__STAT)
check_function_exists(finite HAVE_FINITE)
check_function_exists(fp_class HAVE_FP_CLASS)
check_function_exists(fpclass HAVE_FPCLASS)
check_function_exists(fprintf HAVE_FPRINTF)
check_function_exists(ftime HAVE_FTIME)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(isascii HAVE_ISASCII)
check_function_exists(isinf HAVE_ISINF)
check_function_exists(isnan HAVE_ISNAN)
check_function_exists(isnand HAVE_ISNAND)
check_function_exists(localtime HAVE_LOCALTIME)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(munmap HAVE_MUNMAP)
check_function_exists(printf HAVE_PRINTF)
check_function_exists(putenv HAVE_PUTENV)
check_function_exists(rand HAVE_RAND)
check_function_exists(rand_r HAVE_RAND_R)
check_function_exists(shl_load HAVE_SHLLOAD)
check_function_exists(signal HAVE_SIGNAL)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(sprintf HAVE_SPRINTF)
check_function_exists(srand HAVE_SRAND)
check_function_exists(sscanf HAVE_SSCANF)
check_function_exists(stat HAVE_STAT)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strftime HAVE_STRFTIME)
check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(time HAVE_TIME)
check_function_exists(vfprintf HAVE_VFPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(vsprintf HAVE_VSPRINTF)
check_include_files(pthread.h HAVE_PTHREAD_H)

set(VERSION ${PROJECT_VERSION})
configure_file(
    cmake/template/config.h.in
    ${gen_dir}/config.h
    @ONLY
)

# }
