#!/bin/sh
#
# Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
#
# This file is in the public domain.

set -e
set -u

if [ "${MAKE_TARGET:-}" = sbuild ]
then
	sudo sbuild-createchroot sid --verbose \
		--include=perl \
		/srv/chroot/sid-amd64-sbuild \
		http://ftp.us.debian.org/debian/
fi
