#!bin/bash

let validInput=0
if [$# < 3] 
then
    echo 2> "can't execute. Too few parameters"
elif [$# > 3]
then
    echo 2> "can't execute. Too many parameters"
else
    validInput=1
fi
if [$(find -name $2 -type d) != 0] 
then
    echo 2> "No such directory"
    validInput=0 
fi
if[validInput]
then
        find $2 -type f -name "*.$3" > $1
else
        echo  "correct format:sciptname <filename> <directory> <extension>" 
fi

