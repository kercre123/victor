#! /bin/bash

cd `dirname $0`

if [ "libs.tar.bz2" -nt "include/.haveInclude" ] ; then
	echo "[anki-util] Outdated include directory, removing it"
	rm -rf "include"
	rm -rf "lib"
fi


if [ -a "include/.haveInclude" ] ; then
	echo "[anki-util] Your include directory is just fine"
else
	echo "[anki-util] Unpacking libs.tar.bz2"
    	tar -xjf "libs.tar.bz2"
    	touch "include/.haveInclude"
fi

