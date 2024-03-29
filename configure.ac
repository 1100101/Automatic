#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
m4_define([version_number],0.9)

AC_INIT([Automatic],[version_number],[https://github.com/1100101/Automatic])
AC_SUBST(VERSION_NUMBER,[version_number])

AC_CONFIG_SRCDIR([src/automatic.c])
AM_INIT_AUTOMAKE([1.10 foreign subdir-objects])

# Checks for c compiler.
AC_PROG_CC
AC_PROG_CXX

PKG_PROG_PKG_CONFIG()
if test m4_substr(version_number,3,1) != "x"; then
  supported_build=yes
  if test "x$GCC" = "xyes" ; then
      CFLAGS="-Wdeclaration-after-statement -O3 -funroll-loops"
      CXXFLAGS="-O3 -funroll-loops"
  fi
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
else
  supported_build=no
  if test "x$GCC" = "xyes" ; then
    CFLAGS="-g -Wall -W -Wdeclaration-after-statement -O0 -funroll-loops"
    CXXFLAGS="-g -Wall -W -O0 -funroll-loops"
  fi
fi
AM_CONDITIONAL(DBG_BUILD, test "x$supported_build" = "xno")

AC_PROG_INSTALL
AC_CHECK_TOOL(STRIP, strip)


CURL_MINIMUM=7.15.0
LIBXML2_MINIMUM=2.6.31
PCRE_MINIMUM=7.4
AC_SUBST(CURL_MINIMUM)
AC_SUBST(LIBXML2_MINIMUM)
AC_SUBST(PCRE_MINIMUM)

# export these variable (so Makefile substitutions can be made.)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)

# Checks for libraries.
PKG_CHECK_MODULES(LIBCURL, [libcurl >= $CURL_MINIMUM])
PKG_CHECK_MODULES(LIBXML, [libxml-2.0 >= $LIBXML2_MINIMUM])
PKG_CHECK_MODULES(PCRE, [libpcre >= $PCRE_MINIMUM])
AC_CHECK_LIB(curl, curl_global_init)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([dup2 gettimeofday localtime_r regcomp strerror strstr])


AC_CONFIG_FILES([Makefile src/Makefile src/tests/Makefile])

AC_OUTPUT

