dnl Process this file with autoconf to produce a configure script.
AC_INIT(affsck.c)
AM_INIT_AUTOMAKE(affstools, 0.1a)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(argp.h byteswap.h fcntl.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T

dnl Checks for library functions.
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(strerror)

AC_C_BIGENDIAN
AC_CHECK_SIZEOF(unsigned char)
AC_CHECK_SIZEOF(signed char)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(signed short)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(signed int)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(signed long)

AC_OUTPUT(Makefile)
