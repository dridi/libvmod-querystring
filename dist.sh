#!/bin/sh

if [ $# != 1 ]
then
	cat >&2 <<-EOF
	Usage:
	  $0 <version>
	  $0 <tree-ish>
	EOF
	exit 1
fi

label=libvmod-querystring-$1

if [ $(git tag -l "v$1" | wc -l) = 1 ]
then
	git archive --prefix "$label/" -o "v$1.tar.gz" "v$1"
	exit
fi

git archive --prefix "$label/" -o "$label.tar.gz" "$1"
