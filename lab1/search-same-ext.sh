#!/bin/bash

let validInput=0
if (( $# < 3 )) 
then
    echo "can't execute. Too few parameters" >&2 
elif (( $# > 3 ))
then
    echo "can't execute. Too many parameters" >&2 
else
    validInput=1
fi
if [ ! -d "$2" ] 
then
    echo "No such directory" >&2
    validInput=0 
fi
if ((validInput == 1))
then
        find $2 -type f -name "*.$3" -exec basename {} \; > "$1"
else
        echo  "correct format: sciptname <filename> <directory> <extension>" 
fi

