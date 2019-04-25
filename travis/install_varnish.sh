#!/bin/sh
#
# Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
#
# This file is in the public domain.

set -e
set -u

VERSION=6.0.0

case ${VARNISH_BRANCH:-} in
master)
	wget https://github.com/varnishcache/varnish-cache/archive/master.tar.gz
	tar xf master.tar.gz
	cd varnish-cache-master/
	./autogen.sh
	;;
6.0)
	wget https://github.com/varnishcache/varnish-cache/archive/6.0.tar.gz
	tar xf 6.0.tar.gz
	cd varnish-cache-6.0/
	./autogen.sh
	;;
*)
	wget https://varnish-cache.org/_downloads/varnish-$VERSION.tgz
	tar xf varnish-$VERSION.tgz
	cd varnish-$VERSION/
	;;
esac

./configure --prefix=/usr
make -sj32
sudo make install
