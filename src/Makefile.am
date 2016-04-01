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

AM_CPPFLAGS = @DEFINE_LOGGING@ @VMOD_INCLUDES@
AM_CFLAGS = -Wall -Werror -Wextra

# VMOD library

vmoddir = @VMOD_DIR@
vmod_LTLIBRARIES = libvmod_querystring.la

VCC_FILES = \
	vcc_if.c \
	vcc_if.h

DOC_FILES = \
	vmod_querystring.rst \
	vmod_querystring.man.rst

libvmod_querystring_la_LDFLAGS = \
	-module \
	-export-dynamic \
	-avoid-version \
	-shared

libvmod_querystring_la_SOURCES = \
	vmod_querystring.c \
	vmod_querystring.h

nodist_libvmod_querystring_la_SOURCES = \
	$(VCC_FILES)

$(srcdir)/vmod_querystring.c: $(VCC_FILES)

$(VCC_FILES) $(DOC_FILES): .vcc

.vcc: vmod.vcc
	@echo vmodtool: @VMODTOOL@ $<
	@@VMODTOOL@ $<
	@touch $@

# Documentation

dist_man_MANS = vmod_querystring.3

%.3: %.man.rst
	${RST2MAN} $< $@

# Test suite

VMOD_FILE="$(abs_builddir)/.libs/libvmod_querystring.so"

VTC_LOG_COMPILER = \
	env PATH="@TEST_PATH@" \
	varnishtest -v \
	-Dvmod_querystring="querystring from \"$(VMOD_FILE)\""

TEST_EXTENSIONS = .vtc

TESTS = @VTC_TESTS@

# misc

EXTRA_DIST = \
	vmod.vcc \
	@VTC_TESTS@

CLEANFILES = \
	$(VCC_FILES) \
	$(DOC_FILES) \
	$(dist_man_MANS)
