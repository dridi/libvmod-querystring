#!/bin/sh
#
# Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
#
# This file is in the public domain.

set -e
set -u

VERSION=6.0.0

if [ "${TRAVIS_EVENT_TYPE:-}" = cron ]
then
	wget https://github.com/varnishcache/varnish-cache/archive/master.tar.gz
	tar xf master.tar.gz
	cd varnish-cache-master/
	./autogen.sh
else
	wget https://varnish-cache.org/_downloads/varnish-$VERSION.tgz
	tar xf varnish-$VERSION.tgz
	cd varnish-$VERSION/
fi

./configure --prefix=/usr
make -sj32
sudo make install
