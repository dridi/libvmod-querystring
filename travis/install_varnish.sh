#!/bin/sh
#
# Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
#
# This file is in the public domain.

set -e
set -u

VERSION=6.0.6
BRANCH=${VARNISH_BRANCH:-}

if [ -n "${BRANCH}" ]
then
	wget "https://github.com/varnishcache/varnish-cache/archive/$BRANCH.tar.gz"
	tar xf "$BRANCH.tar.gz"
	cd "varnish-cache-$BRANCH/"
	./autogen.sh
else
	wget https://varnish-cache.org/_downloads/varnish-$VERSION.tgz
	tar xf varnish-$VERSION.tgz
	cd varnish-$VERSION/
fi

./configure --prefix=/usr
make -sj32
sudo make install
