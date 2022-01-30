/*!******************************************************************************************
   File Name           : systemcalls.c
   Author Name         : Amey Sharad Dashaputre
   Compiler            : gcc and aarch64-none-linux-gnu-gcc 
   References		: 1. Linux System Programming book - Chapter 5
   			  2. https://stackoverflow.com/a/13784315/1446624
  
*******************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include "systemcalls.h"


#define FILEMODE 0644

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/

     int retvalue; //return value variable

    retvalue = system(cmd); // fork + exec + wait
    
    if (retvalue == -1) // system error
    {
    	perror("ERROR: system"); 
    	return false;
    }
    printf("System returned successfully!\n"); //success
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/

    int status;
    pid_t pid;
    pid = fork();
    
    if (pid == -1)
    {
    	perror("ERROR: fork"); 
    	return false;
    }
    else if (pid == 0) //child process
    {
    	printf("Child Process successfully created. Inside the child procees with pid %d\n", pid);
    	execv(command[0], command);
    	perror("ERROR : execv"); //If success, execv does not return anything. If it returns, then an error has encountered
    	exit(EXIT_FAILURE); //exit with error
    }
    else 
    {
    	printf("Now inside parent process with pid  %d\n", pid);
    	if(waitpid(pid,&status,0) == -1)	//error
    	{
            perror("ERROR : wait");
            return false;
        }
        
        if ( ! (WIFEXITED(status)) ||  (WEXITSTATUS(status)) )	//error
        {
            perror("ERROR : wait");
            return false;
        }
    }
    va_end(args);
    printf("Do execv succeeded!!\n");
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/
    
    int status;
    pid_t pid;
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, FILEMODE);	//file open
    if(fd == -1)	//error
    {
        perror("open");
        return false;
    }

    pid = fork();
    
    if (pid == -1)	//error
    {
    	perror("ERROR: fork"); 
    	return false;
    }
    else if (pid == 0) //Child process created
    { 
	printf("Child process successfully created. Inside child process with pid %d\n",pid);
        if(dup2(fd,1) < 0)	//error
        {
            perror("dup2"); 
            return false;
        }
        close(fd);
        execv(command[0],command);
    
    	perror("ERROR : execv");	//If success, execv does not return anything. If it returns, then an error has encountered
    	exit(EXIT_FAILURE);
    }
    else
    {
        close(fd);
    	printf("Inside parent process with pid %d\n", pid);
    	if(waitpid(pid,&status,0) == -1) //error
    	{
            perror("wait");
            return false;
        }
        
        if ( ! (WIFEXITED(status)) ||  (WEXITSTATUS(status)) ) 	//error
        {
            perror("Waitstatus");
            return false;
        }
        
    }
    va_end(args);
    printf("Do execv redirect succeeded!!\n");
    return true;
}
