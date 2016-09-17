# Copyright (C) 2012-2016  Dridi Boukelmoune
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

SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4 -I ${VARNISHAPI_DATAROOTDIR}/aclocal

EXTRA_DIST = \
	README.rst \
	debian \
	vmod-querystring.spec

dist_doc_DATA = \
	CHANGELOG \
	CONTRIBUTORS \
	LICENSE

# lcov support

CODE_COVERAGE_OUTPUT_DIRECTORY = lcov
CODE_COVERAGE_IGNORE_PATTERN = "/usr/*"
CODE_COVERAGE_GENHTML_OPTIONS = --prefix $(abs_top_srcdir)

@CODE_COVERAGE_RULES@

lcov: check-code-coverage

# RPM packaging

RPMBUILD = CC=gcc PKG_CONFIG_PATH= rpmbuild -D '_rpmdir rpmbuild' \
	-D '_srcrpmdir rpmbuild/src'
MOCK = mock $(MOCK_OPTS) --resultdir mockbuild

dist: all

rpm: dist rpmbuild-clean
	$(RPMBUILD) -ts $(PACKAGE)-$(VERSION).tar.gz
	$(RPMBUILD) --rebuild rpmbuild/src/$(PACKAGE)-$(VERSION)-*.src.rpm

rpmlint: rpm
	rpmlint $(PACKAGE).spec rpmbuild/*/$(PACKAGE)-*.rpm

mockbuild: dist mockbuild-clean
	$(MOCK) --buildsrpm --sources . --spec $(PACKAGE).spec
	$(MOCK) --rebuild mockbuild/$(PACKAGE)-$(VERSION)-*.src.rpm

rpmbuild-clean:
	rm -rf rpmbuild/

mockbuild-clean:
	rm -rf mockbuild/

clean-local: rpmbuild-clean mockbuild-clean

.PHONY: rpmbuild-clean mockbuild-clean

# DEB packaging

DEB_ORIG = $(PACKAGE)_$(VERSION).orig.tar.gz

$(DEB_ORIG): distdir debuild-clean
	tar -zcf $(PACKAGE)_$(VERSION).orig.tar.gz $(distdir)

deb: $(DEB_ORIG)
	cd $(distdir) && dpkg-buildpackage -rfakeroot -us -uc
	rm -rf $(distdir)

lintian: deb
	lintian $(PACKAGE)_$(VERSION)-1_*.deb

pdebuild: $(DEB_ORIG)
	cd $(distdir) && pdebuild
	rm -rf $(distdir)

debuild-clean:
	rm -f $(PACKAGE)_$(VERSION)*

clean-local: debuild-clean

.PHONY: debuild-clean
