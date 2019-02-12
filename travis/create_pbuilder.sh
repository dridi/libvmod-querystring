#!/bin/sh
#
# Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
#
# This file is in the public domain.

set -e
set -u

if [ "${MAKE_TARGET:-}" = pdebuild ]
then
	sudo pbuilder create --debug
fi
