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

SUBDIRS = . src

ACLOCAL_AMFLAGS = -I m4 -I ${VARNISHAPI_DATAROOTDIR}/aclocal

EXTRA_DIST = \
	README.rst \
	debian \
	vmod-querystring.spec

dist_doc_DATA = \
	CHANGELOG \
	CONTRIBUTORS \
	LICENSE

include lcov.am
include rpm.am
include deb.am

# dirty hack to work around VPATH limitations
all-local:
	$(AM_V_VMODTOOL) cd src && \
	$(PYTHON) $(VMODTOOL) -o vcc_querystring_if vmod_querystring.vcc
