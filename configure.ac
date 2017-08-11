AC_INIT([mines], [0.1], [alan.wayne.johnson@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror -Wno-portability])

AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AM_PROG_AR
AC_PROG_RANLIB

AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])

PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0 gtk+-3.0])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
