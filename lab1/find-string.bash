#!/bin/bash

if [ "$#" -ne 2 ]
then
    echo "wrong arguments" >&2
    echo "use scriptname <string> <dir>"
elif [ ! -d "$2" ]
then
   echo "no such directory" >&2
   echo "aborting" >&2
else
    find $2 -type f -exec grep -r --regexp "$1" {} 2>/dev/null -l  \; | xargs ls -al | sort -k 5 -n | awk '{print $5, $9}'
                                                                                  #-Sr
fi
