#!/bin/bash

#AESD Assignment 1
#Author 	- 	Amey Sharad Dashaputre
#File Name	-  	writer.sh
#References	- 	1. https://stackoverflow.com/questions/6121091/how-to-extract-directory-path-from-file-path
#			2. https://www.cyberciti.biz/faq/unix-linux-test-existence-of-file-in-bash/
#			3. https://linuxhint.com/write-to-file-bash/

#store the input arguments into local varibles

writefile=$1
writestr=$2

#check if the number of arguments are equal to two or not

if [ $# -ne 2 ] 
then
	echo " Invalid number of agruments. There should be total 2 arguments."
	echo " The first argument should be the File Directory Path."
	echo " The second argument should be the string to be searched in the specified directory path."
	exit 1
fi

#get name of directory from the filepath

directoryname="${writefile%/*}"

#if directory does not exists, create a new one

if [ ! -d "$directoryname" ]  
then
	echo "Creating a new directory"
	mkdir -p $directoryname
	
fi

#Create the file in the directory or overwrite the file if it exists already

touch $writefile

#check if file is successfully created or not

if [ -f "$writefile" ] 
then
	#write the string in the file
	
	echo "$writestr" >  "$writefile"
else
	echo "Error in creating the file"
	exit 1
fi

#end
