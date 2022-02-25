#!/bin/bash

if [ "$#" -ne 2 ]
then
    echo "wrong arguments" >&2
    echo "use scriptname <string> <dir>"
else
   find $2 -type f -exec grep -H "$1" {} \; | sort -M -n | ls -l
  
    
   