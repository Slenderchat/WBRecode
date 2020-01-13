#!/bin/bash
GC="git clone -j $(nproc) --single-branch"
if [ ! -e $1/.git/logs/HEAD ]
then
	if [ $# -eq 2 ]
	then
			${GC} $2 temp_$(basename $1)
	elif [ $# -ge 3 ]
	then
		${GC} $2 temp_$(basename $1)
		pushd temp_$(basename $1)
		_1=$1
		shift 2
		for mod in "$@"
		do
			git submodule update --init $mod
		done	
		popd
		set ${_1}
	fi
	cp -rf temp_$(basename $1)/. $1
	rm -rf temp_$(basename $1)
fi
