#!/bin/bash

if (( $# != 2 ));
then
	echo "usage: <output> <source>"
	exit 1
fi

out=$(realpath "$1")
src=$(realpath "$2")

pushd "$src"

files=""

for i in *;
do
	files="$files $i"
done

tar cf "$out" $files
popd
