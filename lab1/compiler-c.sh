#!/bin/bash


if [ "$#" -ne 2 ]
then
    echo "improper amount of arguments" >&2
    echo "aborting" >&2
    echo "use: scriptname <source file> <out file>"
else
   # let source=$1
   # let exec=$2
    if [ -f "$1" ]
    then
	gcc $1 -o $2 && ./$2
#	if [ "$?" -ne 0 ]
#	then
#	    echo "successful"
#	    #chmod o+rwx $2
#	    ./$2
#	else
#	    echo "compilation failed" >&2
#	fi
    else
	echo "source file doesn't exist" >&2
    fi
fi
	    
	   
