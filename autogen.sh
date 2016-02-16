#!/bin/sh

set -e

PLATFORM="$(uname -s)"

case "$PLATFORM" in
FreeBSD|Linux|SunOS)
	LIBTOOLIZE=libtoolize
	;;
Darwin)
	LIBTOOLIZE=glibtoolize
	;;
*)
	echo "WARNING: unrecognized platform: $PLATFORM" >&2
	LIBTOOLIZE=libtoolize
esac

mkdir -p m4

aclocal -I m4
$LIBTOOLIZE --copy --force
autoheader
automake --add-missing --copy --foreign
autoconf
