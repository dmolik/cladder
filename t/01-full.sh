#!/bin/bash
source ${srcdir:-.}/t/lib.sh

mkdir ${ROOT}/data
echo "hello world" > ${ROOT}/data/file1

./cladder ${ROOT}/data ${ROOT}/sandbox  > ${ROOT}/log 2>&1

diag_file ${ROOT}/log

string_is $(cat ${ROOT}/sandbox/file1) "hello world" \
	"File contents do not match"

exit 0
