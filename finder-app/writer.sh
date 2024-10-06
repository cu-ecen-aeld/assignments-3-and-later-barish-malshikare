#!/bin/sh
#echo $1 
#echo $2

if [ $# -ne 2 ]
then
	echo usage: ./writer.sh file_to_update input_text
	exit 1
fi
mkdir -p "$(dirname "$1")"
echo $2>$1

if [ ! $? -eq 0 ]
then 
	exit 1
fi
