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

# QS_ARG_ENABLE(feature, default)
# -------------------------------
AC_DEFUN([QS_ARG_ENABLE], [dnl
	AC_ARG_ENABLE([$1],
		[AS_HELP_STRING([--enable-$1], [enable $1 (default is $2)])],
		[],
		[enable_[]$1=$2])
])

# _QS_CHECK_CFLAGS
# ----------------
AC_DEFUN_ONCE([_QS_CHECK_CFLAGS], [

qs_save_CFLAGS="$CFLAGS"
qs_CFLAGS=

qs_check_cflag() {
	CFLAGS="$qs_CFLAGS $[]1"

	mv confdefs.h confdefs.h.orig
	touch confdefs.h

	AC_MSG_CHECKING([whether the compiler accepts $[]1])
	AC_RUN_IFELSE(
		[AC_LANG_SOURCE([int main(void) { return (0); }])],
		[qs_result=yes],
		[qs_result=no])
	AC_MSG_RESULT($qs_result)

	rm confdefs.h
	mv confdefs.h.orig confdefs.h

	test "$qs_result" = yes
}

qs_check_cflags() {
	for _cflag
	do
		if qs_check_cflag $_cflag
		then
			qs_CFLAGS="$qs_CFLAGS $_cflag"
		fi
	done
}

])

# QS_CHECK_CFLAGS(CFLAGS)
# -----------------------
AC_DEFUN([QS_CHECK_CFLAGS], [dnl
AC_REQUIRE([_QS_CHECK_CFLAGS])dnl
qs_check_cflags m4_normalize([$1])
CFLAGS="$qs_CFLAGS $qs_save_CFLAGS"
])

# QS_FORCE_CFLAGS(CFLAGS)
# -----------------------
AC_DEFUN([QS_FORCE_CFLAGS], [
AC_REQUIRE([_QS_CHECK_CFLAGS])dnl
qs_save_CFLAGS="$qs_save_CFLAGS m4_normalize([$1])"
CFLAGS="$qs_CFLAGS $qs_save_CFLAGS"
])

# QS_CHECK_PROG(VARIABLE, PROGS)
# ------------------------------
AC_DEFUN([QS_CHECK_PROG], [
AC_CHECK_PROGS([$1], [$2], [no])
test "$[$1]" = no &&
AC_MSG_ERROR([Could not find program $2])
])
