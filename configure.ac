# libvmod-querystring - querystring manipulation module for Varnish
# 
# Copyright (C) 2012-2014, Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
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
AC_COPYRIGHT([Copyright (c) 2012-2014 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>])
AC_INIT([libvmod-querystring], [0.2], [https://github.com/dridi/libvmod-querystring/issues])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/vmod_querystring.c)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_SYSTEM
AC_LANG(C)

AM_INIT_AUTOMAKE([foreign])

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_CC_STDC
if test "x$ac_cv_prog_cc_c99" = xno; then
	AC_MSG_ERROR([Could not find a C99 compatible compiler])
fi
AC_PROG_CPP

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

# Check for rst utilities
AC_CHECK_PROGS(RST2MAN, [rst2man rst2man.py], "no")
if test "x$RST2MAN" = "xno"; then
	AC_MSG_WARN([rst2man not found - not building man pages])
fi
AM_CONDITIONAL(HAVE_RST2MAN, [test "x$RST2MAN" != "xno"])

# Check for pkg-config
PKG_PROG_PKG_CONFIG

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/stdlib.h])

# Check for functions
AC_CHECK_FUNCS([mempcpy])

# Check for python
AC_CHECK_PROGS(PYTHON, [python3 python3.1 python3.2 python2.7 python2.6 python2.5 python2 python], [AC_MSG_ERROR([Python is needed to build this vmod, please install python.])])

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

# Varnish source tree
AC_ARG_VAR([VARNISHSRC], [path to Varnish source tree (mandatory)])
if test "x$VARNISHSRC" = x; then
	AC_MSG_ERROR([No Varnish source tree specified])
fi

VARNISHSRC=`cd $VARNISHSRC && pwd`
Varnish_Version_MAJOR=

# Is it a Varnish 3 or 4 source tree ?
AC_CHECK_FILE([$VARNISHSRC/lib/libvmod_std/vmod.py], [Varnish_Version_MAJOR=3])
AC_CHECK_FILE([$VARNISHSRC/lib/libvcc/vmodtool.py],  [Varnish_Version_MAJOR=4])

AC_SUBST([VARNISH_MAJOR], [$Varnish_Version_MAJOR])
AC_SUBST([DEFINE_MAJOR], [-DVARNISH_MAJOR=$Varnish_Version_MAJOR])

# Can we build a VMOD with VARNISHSRC ?
AS_CASE(["$Varnish_Version_MAJOR"],
	[4], [AC_SUBST([VMODTOOL], ['$(VARNISHSRC)/lib/libvcc/vmodtool.py'])],
	[3], [AC_SUBST([VMODTOOL], ['$(VARNISHSRC)/lib/libvmod_std/vmod.py'])],
	     [AC_MSG_FAILURE(["$VARNISHSRC" is not a Varnish source directory])]
)

# Check that varnishtest is built in the varnish source directory
AC_CHECK_FILE([$VARNISHSRC/bin/varnishtest/varnishtest],
	[],
	[AC_MSG_FAILURE([Could not find "$VARNISHSRC/bin/varnishtest/varnishtest". Please build your varnish source directory])]
)

# Check vanish ABI version
AC_CHECK_FILE([$VARNISHSRC/include/vmod_abi.h],
	[
		AC_MSG_CHECKING([varnish ABI version])
		[VMOD_ABI_Version=`cut -d ' ' -f 4 "$VARNISHSRC/include/vmod_abi.h"`]
		AC_MSG_RESULT([$VMOD_ABI_Version])
	],
	[
		[VMOD_ABI_Version=3.0.0]
		AC_MSG_NOTICE([Could not find "$VARNISHSRC/include/vmod_abi.h". Assuming varnish 3.0.0 ABI version])
	]
)

AS_CASE([`cat $VARNISHSRC/include/vmod_abi.h 2>/dev/null`],
	[*4.0.0*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_4_0_0])],
	[*3.0.6*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_6])],
	[*3.0.5*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_5])],
	[*3.0.4*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_4])],
	[*3.0.3*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_3])],
	[*3.0.2*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_2])],
	[*3.0.1*], [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_1])],
	           [AC_SUBST([DEFINE_ABI], [-DHAVE_VARNISH_3_0_0])]
)

# vmod installation dir
AC_ARG_VAR([VMODDIR], [vmod installation directory @<:@LIBDIR/varnish/vmods@:>@])
if test "x$VMODDIR" = x; then
	VMODDIR=`pkg-config --variable=vmoddir varnishapi`
	if test "x$VMODDIR" = x; then
		AC_MSG_FAILURE([Could not determine vmod installation directory])
	fi
fi

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT

