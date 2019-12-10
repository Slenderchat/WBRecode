#!/bin/sh
GC="git clone --single-branch -j $(nproc)"
if [ ! -e $1/.git/logs/HEAD ]
then
	if [ $# -eq 2 ]
	then
			${GC} $2 temp_$(basename $1)
	elif [ $# -eq 3 ]
	then
			${GC} --branch $3 $2 temp_$(basename $1)
	elif [ $# -ge 4 ]
	then
		${GC} --branch $3 $2 temp_$(basename $1)
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
	cp -rf temp_$(basename $1)/. $1
	rm -rf temp_$(basename $1)
fi