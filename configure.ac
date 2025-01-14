#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

dnl Minimum Autoconf version
AC_PREREQ([2.69])
dnl configure.ac initialization
dnl we use some macros that we are in m4 directory
AC_INIT([br], [1.7], [gratien.dhaese at google.com], [], [https://github.com/gdha/br])
dnl Where are the sources?
AC_CONFIG_SRCDIR([br.c])
dnl What header will be generated by configure script
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Where to put various auxiliary files
AC_CONFIG_AUX_DIR([libltdl/config])
dnl Automake init with all warining reported
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for libraries.
# AC_CHECK_LIB (library, function, [action-if-found], [action-if-not-found], [other-libraries])
# FIXME: Replace `main' with a function in `-lcrypto':
AC_CHECK_LIB([crypto], [EVP_EncryptInit], [],
             [AC_MSG_FAILURE([can't find openssl crypto lib])]
	     [])
# FIXME: Replace `main' with a function in `-lssl':
#AC_CHECK_LIB([main])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h grp.h br.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_MACRO_DIR([libltdl/m4])

# Check for ASCIIDOC
AC_ARG_VAR([ASCIIDOC], [asciidoc path])
AC_PATH_PROG([ASCIIDOC], [asciidoc], [])
if test -z "$ASCIIDOC"
then
  AC_MSG_WARN([asciidoc not found, man pages rebuild will not be possible])
fi
AM_CONDITIONAL([USE_ASCIIDOC], [test -n "$ASCIIDOC"])

# Check for XMLTO
AC_ARG_VAR([XMLTO], [xmlto path])
AC_PATH_PROG([XMLTO], [xmlto], [])
if test -z "$XMLTO"
then
   AC_MSG_WARN([xmlto not found, man pages rebuild will not be possible])
fi


LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT
LTDL_INIT

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  Br.............: version $PACKAGE_VERSION
  Prefix.........: $prefix
  C compiler.....: $CC $CFLAGS $CPPFLAGS
  Asciidoc.......: ${ASCIIDOC:-NONE}

  Now type 'make @<:@<target>@:>@'
    where the optional <target> is:
      all                - build all binaries
      install            - install everything
      rpm                - build RPM package
      deb                - build DEB package
"

