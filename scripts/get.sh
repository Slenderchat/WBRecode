#!/bin/bash
GC="git clone -j $(nproc) --single-branch"
if [ ! -e $1/.git/logs/HEAD ]
then
	if [ $# -eq 3 ]
	then
			${GC} $2 temp_$(basename $1) -b $3
	elif [ $# -ge 4 ]
	then
		${GC} $2 temp_$(basename $1) -b $3
		pushd temp_$(basename $1)
		_1=$1
		shift 3
		for mod in "$@"
		do
			git submodule update --init $mod
		done	
		popd
		set ${_1}
	fi
	cp -r temp_$(basename $1)/. $1
	rm -rf temp_$(basename $1)
fi
