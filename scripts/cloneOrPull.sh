#!/bin/sh
GC="git clone --single-branch -j $(nproc)"
case $# in
	2)
		${GC} $2 temp_$(basename $1)
		;;
	3)
		${GC} --branch $3 $2 temp_$(basename $1)
		;;
esac
if [ $# -ge 4 ]
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
pushd $1
git pull --recurse-submodules -j $(nproc)
popd
