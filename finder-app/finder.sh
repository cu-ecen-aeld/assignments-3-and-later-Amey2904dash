#!/bin/bash
#Author- Amey Sharad Dashaputre
#AESD Assignment 1
#File Name- finder.sh




filesdir=$1
serachstr=$2

#check if the number of arguments are correct
if [ $# -ne 2 ] 
then
	echo "The number of arguments passed are incorrect. Please provide two agruments."
	exit 1
fi

if [ ! -d "$filesdir" ] 
then
	echo "Directory dose not represnt a directory on the filesystem."
	exit 1

else
	#Calculate the number of files
	X=$(find "$filesdir" -type f | wc -l)
	#Count number of matching lines in the files with the input string
	Y=$(grep -r "$serachstr" "$filesdir" | wc -l)
fi

#print the result
	echo "The number of files are $X and the number of matching lines are $Y"






#end
