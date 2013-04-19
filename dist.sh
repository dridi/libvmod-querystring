#!/bin/sh

if [ $# != 1 ]
then
	echo -e "Usage:\n $0 <version>\n $0 <tree-ish>" >&2
	exit 1
fi

label=libvmod-querystring-$1

if [ `git tag -l $label | wc -l` = 1 ]
then
	git archive --prefix $label/ -o $label.tar.gz $label
	exit
fi

git archive --prefix $label/ -o $label.tar.gz $1

