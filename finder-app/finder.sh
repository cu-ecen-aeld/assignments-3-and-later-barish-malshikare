#!/bin/bash

if [ $# -ne 2 ]
then	
	echo "finder.sh </path/to/dir/>  <string_to_search>"
	exit 1
fi
if [ ! -d "$1" ]; then
  echo "filesdir does not represent a directory on the filesystem"
  exit 1
fi
files=$(find $1 -type f | wc -l)
lines=$(grep -r $2 $1 | wc -l)
echo "The number of files are $files and the number of matching lines are $lines"
