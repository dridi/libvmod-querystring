# libvmod-querystring - querystring manipulation module for Varnish
#
# Copyright (C) 2012-2016, Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above
#    copyright notice, this list of conditions and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials
#    provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

AC_PREREQ(2.59)
AC_COPYRIGHT([Copyright (c) 2012-2016 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>])
AC_INIT(
	[libvmod-querystring],
	[0.4],
	[https://github.com/dridi/libvmod-querystring/issues])

AC_CONFIG_MACRO_DIR([m4])
m4_ifndef(
	  [VARNISH_VMOD_INCLUDES],
	  AC_MSG_ERROR([Need varnish.m4 -- see README.rst]))

AC_CONFIG_SRCDIR(src/vmod.vcc)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_SYSTEM
AC_LANG(C)

AM_INIT_AUTOMAKE([foreign])

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CPP
if test "x$ac_cv_prog_cc_c99" = xno; then
	AC_MSG_ERROR([Could not find a C99 compatible compiler])
fi

AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

# Check for rst utilities
AC_ARG_WITH([rst2man],
AS_HELP_STRING(
	[--with-rst2man=PATH],
	[Location of rst2man (auto)]),
	[RST2MAN="$withval"],
	AC_CHECK_PROGS(RST2MAN, [rst2man rst2man.py], [no]))
if test "$RST2MAN" = "no"; then
	AC_MSG_ERROR(
		[rst2man is needed to build the manual. To proceed without, specify --with-rst2man=true.])
fi

# Test suite
VTC_TESTS="$(cd $srcdir/src && echo tests/*.vtc)"
AC_SUBST(VTC_TESTS)

# Check for pkg-config
PKG_PROG_PKG_CONFIG

# Checks for header files
AC_HEADER_STDC

# Check for functions
AC_CHECK_FUNCS([mempcpy])

# Check for python
AC_CHECK_PROGS(PYTHON,
	[python python3 python2],
	[AC_MSG_ERROR([Python is needed to build this vmod.]) ])

# read Varnis's pkg-config
PKG_CHECK_MODULES([libvarnishapi], [varnishapi])
PKG_CHECK_VAR([LIBVARNISHAPI_PREFIX], [varnishapi], [prefix])
PKG_CHECK_VAR([LIBVARNISHAPI_DATAROOTDIR], [varnishapi], [datarootdir])
PKG_CHECK_VAR([LIBVARNISHAPI_BINDIR], [varnishapi], [bindir])
PKG_CHECK_VAR([LIBVARNISHAPI_SBINDIR], [varnishapi], [sbindir])
AC_SUBST([LIBVARNISHAPI_DATAROOTDIR])

# Varnish include files tree
VARNISH_VMOD_INCLUDES
VARNISH_VMOD_DIR
VARNISH_VMODTOOL

# inherit the prefix from Varnish.
# acessing ac_ variable because AC_PREFIX_DEFAULT acts too early
ac_default_prefix=$LIBVARNISHAPI_PREFIX

AC_SUBST([TEST_PATH], [$LIBVARNISHAPI_SBINDIR:$LIBVARNISHAPI_BINDIR:$PATH])

# Conditional logging
AC_ARG_ENABLE(
	[logging],
	[AS_HELP_STRING(
		[--disable-logging],
		[Disable VCL calls and return values logging. @<:@enabled by default@:>@]
	)],
	[],
	[enable_logging=yes]
)

if test "x$enable_logging" != "xno"; then
	AC_SUBST([DEFINE_LOGGING], [-DQS_ENABLE_LOGGING])
fi

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT
