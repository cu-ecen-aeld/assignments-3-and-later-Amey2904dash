#!/bin/bash
#Author- Amey Sharad Dashaputre
#AESD Assignment 1
#File Name- writer.sh




writefile=$1
writestr=$2

#check if the number of arguments are correct
if [ $# -ne 2 ] 
then
	echo "The number of arguments passed are incorrect. Please provide two agruments."
	exit 1
fi

#extract name of file from the filepath
directoryname="${writefile%/*}"

#if directory does not exists, create a new one

if [ ! -d "$directoryname" ]  
then
	echo "Creating a new directory"
	mkdir -p $directoryname
	
fi
touch $writefile
if [ -f "$writefile" ] 
then

	echo "$writestr" >  "$writefile"
else
	echo "Error creating the file"
	exit 1
fi
#end
