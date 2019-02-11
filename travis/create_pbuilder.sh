#!/bin/sh
#
# Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
#
# This file is in the public domain.

set -e
set -u

if [ "${MAKE_TARGET:-}" = pdebuild ]
then
	sudo pbuilder create \
		--distribution sid \
		--mirror http://ftp.us.debian.org/debian/ \
		--debootstrapopts \
		"--keyring=/usr/share/keyrings/debian-archive-keyring.gpg"
fi
