AC_PREREQ([2.69])
AC_INIT([i2c_access], [1.3], [james.covey-crump@spx.com])
AC_CONFIG_SRCDIR([src/i2c_access.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability])
PKG_PROG_PKG_CONFIG

# Remember externally set CFLAGS
EXTERNAL_CFLAGS="$CFLAGS"

# Checks for programs. These may set default variables, such as CFLAGS
AC_PROG_CPP
AC_PROG_CC_C99
AC_PROG_RANLIB

# Reset the externally set CFLAGS after calling AC_PROG*
CFLAGS="$EXTERNAL_CFLAGS"

# Use the C language and compiler for the following checks
AC_LANG([C])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0], [have_libglib=yes], [have_libglib=no])
AM_CONDITIONAL([GLIB],  [test "$have_libglib" = "yes"])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Add debug support
AC_ARG_ENABLE(debug,
  AS_HELP_STRING(
    [--enable-debug],
    [enable debugging, default: no]),
    [case "${enableval}" in
      yes) debug=true ;;
      no)  debug=false ;;
      *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],
    [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF(DEBUG,
    AC_DEFINE(DEBUG, 1, [Define to 0 if this is a release build]),
    AC_DEFINE(DEBUG, 0, [Define to 1 or higher if this is a debug build]))


# Checks for library functions.
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
