AC_INIT([mines], [0.1], [alan.wayne.johnson@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AX_CXX_COMPILE_STDCXX(11, noext, mandatory)
AX_CXX_COMPILE_STDCXX(14, noext, optional)

AC_PROG_CXX
AM_PROG_AR
AC_PROG_RANLIB

PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
