/*!******************************************************************************************
   File Name           : writer.c
   Author Name         : Amey Sharad Dashaputre
   Compiler            : gcc and aarch64-none-linux-gnu-gcc 
  References		:https://stackoverflow.com/questions/23092040/how-to-open-a-file-which-overwrite-existing-content/23092113
  
*******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/*MACROS*/
#define NUMBER_OF_ARGS 3
#define FILE_OPEN_MODE 0644

/*Main Function*/
int main(int argc, char * argv[])
{
	int fd;
	char * writefile;
   	char * writestr;
   	int writestrLen;
   	int noOfBytesWritten;
	
	openlog(NULL, 0, LOG_USER); /*Initialize the syslog daemon*/
	
	if(NUMBER_OF_ARGS != argc)	//check if the number of arguments are equal to two or not
	{
		syslog(LOG_INFO, " Invalid number of agruments. There should be total 2 arguments.\n");
		syslog(LOG_INFO, " The first argument should be the File Directory Path.\n");
		syslog(LOG_INFO, " The second argument should be the string to be searched in the specified directory path.\n");
		syslog(LOG_ERR, "Invalid number of arguments %d\n", argc);
		exit(1);
	}
	
	/*store the input arguments into local varibles*/
   	writefile   = argv[1];
   	writestr    = argv[2];
   	writestrLen = strlen(argv[2]);
   	
	
   	fd = open(writefile, O_RDWR | O_CREAT | O_TRUNC, 0644); /*opens or creates a new file*/
   	
   	if(fd == -1)	//Error
   	{
   		syslog(LOG_ERR, "Error in creating or opening the file");
   		exit(1);  
   	}
   	noOfBytesWritten =  write( fd, writestr, writestrLen);	// Write the string into the file
   	
   	if(noOfBytesWritten == -1)	//Error
   	{
   		syslog(LOG_ERR, "File Write unsuccessful");
    		
   	}
   	else if(noOfBytesWritten != writestrLen)	//partial file write
    	{
	    	// error message
	    	syslog(LOG_ERR, "String is  partially written");
	    	
    	}
    	else	//Successfull write
    	{
    		syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    	}
    	
    	if(close(fd) == -1 )	//Close file
    	{
    		syslog(LOG_ERR, "Error closing the file"); //Erroe while closing
    		exit(1);
    	}
    	else	//File Closed successfull
    	{
    		syslog(LOG_DEBUG, "File closed successfully");	
    	}
    	closelog();
    	return 0;
}
