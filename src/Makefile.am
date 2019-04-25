# Copyright (C) 2012-2018  Dridi Boukelmoune
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

AM_CFLAGS = $(VARNISHAPI_CFLAGS)
AM_LDFLAGS = $(VMOD_LDFLAGS)

if LCOV_ENABLED
AM_CPPFLAGS = -DWITHOUT_ASSERTS
endif

# VMOD library

vmod_LTLIBRARIES = libvmod_querystring.la

dist_man_MANS = vmod_querystring.3

libvmod_querystring_la_SOURCES = \
	vmod_querystring.c

nodist_libvmod_querystring_la_SOURCES = \
	vcc_querystring_if.c \
	vcc_querystring_if.h

@BUILD_VMOD_QUERYSTRING@

# Documentation

pdf-local: vmod_querystring.pdf

.rst.pdf:
	rst2pdf $< $@

# Test suite

AM_TESTS_ENVIRONMENT = \
	PATH="$(VMOD_TEST_PATH):$(PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH):$(LD_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = -p vmod_path="$(abs_builddir)/.libs:$(VARNISHAPI_VMODDIR)"

TESTS = \
	tests/e00001.vtc \
	tests/f00001.vtc \
	tests/f00002.vtc \
	tests/f00003.vtc \
	tests/f00004.vtc \
	tests/f00005.vtc \
	tests/f00006.vtc \
	tests/f00007.vtc \
	tests/f00008.vtc \
	tests/g00001.vtc \
	tests/m00001.vtc \
	tests/p00001.vtc \
	tests/p00002.vtc \
	tests/p00003.vtc \
	tests/r00022.vtc \
	tests/r00026.vtc \
	tests/w00001.vtc

# misc

EXTRA_DIST = vmod_querystring.vcc $(TESTS)
CLEANFILES = vmod_querystring.pdf
