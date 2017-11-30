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
	[1.0.3],
	[https://github.com/dridi/libvmod-querystring/issues],
	[vmod-querystring],
	[https://github.com/dridi/libvmod-querystring])
AC_COPYRIGHT([Copyright (C) 2012-2017  Dridi Boukelmoune])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/vmod_querystring.vcc])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([1.12 -Wall -Werror foreign parallel-tests])
AM_PROG_AR

LT_PREREQ([2.2.6])
LT_INIT([dlopen disable-static])

QS_ARG_ENABLE([warnings], [no])
QS_ARG_ENABLE([lcov], [no])

AX_PTHREAD(, [AC_MSG_ERROR([Could not configure pthreads support])])

if test "$enable_warnings" != no
then
	# FreeBSD's WARNS level 6
	QS_CHECK_CFLAGS([
		-Werror
		-Wall
		-W
		-Wstrict-prototypes
		-Wmissing-prototypes
		-Wpointer-arith
		-Wreturn-type
		-Wcast-qual
		-Wwrite-strings
		-Wswitch
		-Wshadow
		-Wunused-parameter
		-Wcast-align
		-Wchar-subscripts
		-Winline
		-Wnested-externs
		-Wredundant-decls
		-Wold-style-definition
		-Wmissing-variable-declarations
	])

	# Other desirable warnings
	QS_CHECK_CFLAGS([
		-Wextra
		-Wmissing-declarations
		-Wredundant-decls
		-Wsign-compare
		-Wunused-result
		-Wformat-security
	])

	# Varnish API breakage still ABI-compatible
	QS_CHECK_CFLAGS([-Wno-int-conversion])
fi

AM_CONDITIONAL([LCOV_ENABLED], [test "$enable_lcov" != no])
if test "$enable_lcov" != no
then
	QS_CHECK_PROG([LCOV], [lcov])
	QS_CHECK_PROG([GENHTML], [genhtml])
	QS_FORCE_CFLAGS([-O0 -g -fprofile-arcs -ftest-coverage])
fi

AC_ARG_WITH([rst2man],
	AS_HELP_STRING(
		[--with-rst2man=PATH],
		[Location of rst2man (auto)]),
	[RST2MAN="$withval"],
	AC_CHECK_PROGS(RST2MAN, [rst2man rst2man.py], []))

# To build from git, Varnish >= 5.1.1 is needed
VARNISH_PREREQ([4.1.3])
VARNISH_VMODS([querystring])

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
")
