#!/bin/bash

#AESD Assignment 1
#Author 	- 	Amey Sharad Dashaputre
#File Name	-  	finder.sh
#References	- 	1. https://devconnected.com/how-to-count-files-in-directory-on-linux/
#			2. https://stackoverflow.com/questions/16956810/how-do-i-find-all-files-containing-specific-text-on-linux

#store the input arguments into local varibles

filesdir=$1
serachstr=$2

#check if the number of arguments are equal to two or not

if [ $# -ne 2 ] 
then
	echo " Invalid number of agruments. There should be total 2 arguments."
	echo " The first argument should be the File Directory Path."
	echo " The second argument should be the string to be searched in the specified directory path."
	exit 1
fi

#check if the directory passed as the first argument exists or not.

if [ ! -d "$filesdir" ] 
then
	echo "Directory- $filesdir dose not represnt a directory on the FileSystem!"
	exit 1

else
	#Calculate the number of files in the directory
	
	X=$(find "$filesdir" -type f | wc -l)
	
	#Count number of matching lines in the files with the input string
	
	Y=$(grep -r "$serachstr" "$filesdir" | wc -l)
fi

#print the result

echo "The number of files are $X and the number of matching lines are $Y"

#end
