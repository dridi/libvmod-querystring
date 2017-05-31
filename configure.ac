# Copyright (C) 2012-2017  Dridi Boukelmoune
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.68])
AC_INIT([libvmod-querystring],
	[1.0.2],
	[https://github.com/dridi/libvmod-querystring/issues],
	[vmod-querystring],
	[https://github.com/dridi/libvmod-querystring])
AC_COPYRIGHT([Copyright (C) 2012-2017  Dridi Boukelmoune])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/vmod_querystring.vcc])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([1.12 -Wall -Werror foreign parallel-tests])
AM_SILENT_RULES([yes])
AM_PROG_AR

LT_PREREQ([2.2.6])
LT_INIT([dlopen disable-static])

ax_enable_compile_warnings=error
AX_CODE_COVERAGE
AX_COMPILER_FLAGS_CFLAGS
AX_PTHREAD(, [AC_MSG_ERROR([Could not configure pthreads support])])

AC_ARG_WITH([rst2man],
	AS_HELP_STRING(
		[--with-rst2man=PATH],
		[Location of rst2man (auto)]),
	[RST2MAN="$withval"],
	AC_CHECK_PROGS(RST2MAN, [rst2man rst2man.py], []))

# To build from git, Varnish >= 5.1.1 is needed
VARNISH_PREREQ([4.1.3])
VARNISH_VMODS([querystring])

AC_ARG_ENABLE([dev-flags],
	AS_HELP_STRING(
		[--enable-dev-flags],
		[Enable additional flags for development]),
	[dev_flags="$withval"],
	[dev_flags=no])

if test "$dev_flags" != no
then
	AS_VERSION_COMPARE([$VARNISH_VERSION], [5.1],
		[CFLAGS="$CFLAGS -Wint-conversion"],
		[CFLAGS="$CFLAGS -Wno-int-conversion"],
		[CFLAGS="$CFLAGS -Wno-int-conversion"])
fi

AC_CONFIG_FILES([
	Makefile
	debian/control
	debian/copyright
	src/Makefile
	vmod-querystring.spec
])

AC_OUTPUT

AS_ECHO("
	==== $PACKAGE_STRING ====

	varnish:      $VARNISH_VERSION
	prefix:       $prefix
	vmoddir:      $vmoddir

	compiler:     $CC
	cflags:       $CFLAGS
	ldflags:      $LDFLAGS
	warnings:     $WARN_CFLAGS
")
