#!/bin/sh

if [ $# != 1 ]
then
	echo -e "Usage:\n $0 <version>\n $0 <tree-ish>" >&2
	exit 1
fi

label=libvmod-querystring-v$1

if [ `git tag -l v$1 | wc -l` = 1 ]
then
	git archive --prefix $label/ -o v$1.tar.gz v$1
	exit
fi

git archive --prefix $label/ -o $label.tar.gz $1

