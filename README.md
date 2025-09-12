# Sources of the tool br

## How to build br from sources

 
### Pre-requisite software we need to perform a succesful compilation and RPM package build

```bash
rpm -q autoconf  (if not present install it)
rpm -q automake (if not present install it)
rpm -q libtool (if not present install it)
rpm -q libtool-ltdl-devel (if not present install it)
rpm -q rpm-build (if not present install it)
rpm -q rpm-sign (if not present install it)
rpm -q openssl-devel (if not present install it)
rpm -q asciidoc (if not present install it)
rpm -q xmlto (if not present install it)
```

### Create the input file for make

Create Makefile.am

```bash
$ cat Makefile.am

AM_CFLAGS                       = -Wall -Wextra -O2

bin_PROGRAMS                    = br
br_SOURCES                      = br.c
br_LDADD                        =
ACLOCAL_AMFLAGS                 = -I libltdl/m4
EXTRA_DIST                      = br.spec
```

### Create the input file for the configure tool

Create configure.ac


Be aware that the package version number is defined in variable AC_INIT
(see [1.0])
and also update the Version in br.spec!!

```bash
$ cat configure.ac
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([br], [1.0], [gratien.dhaese at gmail.com])
AC_CONFIG_SRCDIR([br.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CONFIG_AUX_DIR([libltdl/config])
AM_INIT_AUTOMAKE

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypto':
AC_CHECK_LIB([main])
# FIXME: Replace `main' with a function in `-lssl':
AC_CHECK_LIB([main])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_MACRO_DIR([libltdl/m4])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT
LTDL_INIT

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
```

### Run autoheader to retrieve the needed files

```bash
$ autoheader \
     && aclocal \
     && libtoolize --ltdl --copy --force \
     && automake --add-missing --copy \
     && autoconf

$ autoreconf -vfi
```

### Run configure

```bash
$ ./configure
```

Check if install-sh is present

```bash
$ ls -l install-sh
```

If not present copy it from ./libltdl/config/install-sh

### Run make


```bash
$ make 
$ make rpm
```

## Bug reports

Send mail to the Author(s) - see file AUTHORS
