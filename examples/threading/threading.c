/*!******************************************************************************************
   File Name           : threading.c
   Author Name         : Amey Sharad Dashaputre
   Compiler            : gcc and aarch64-none-linux-gnu-gcc 
   References		: 1. Lecture Videos and slidea of Prof Dan Walkes
   			  2. Linux System Programming book - Chapter 7
  
*******************************************************************************************/

#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
	
	DEBUG_LOG("executing thread functionality\n");
    	struct thread_data* thread_func_args = (struct thread_data *) thread_param;		 //get thread parameter
    	thread_func_args->thread_complete_success = true;		 //set to true by default
    	int retVal = usleep(thread_func_args->wait_to_obtain_ms*1000);	//sleep before locking on mutex
	
	 if(retVal == -1)
	 {
	    	ERROR_LOG("usleep for mutex obtain failed");
	    	thread_func_args->thread_complete_success = false;
	    	return thread_param;
    	}
    

    	retVal = pthread_mutex_lock(thread_func_args->mutex_thread);	    //lock on mutex
    
    	if(retVal !=0 )
    	{
	    ERROR_LOG("pthread_mutex_lock failed");
	    thread_func_args->thread_complete_success = false;
	    return thread_param;
    	}
    
    	retVal = usleep(thread_func_args->wait_to_release_ms*1000);	  //sleep before unlocking mutex
    
    	if(retVal == -1)
    	{
	    ERROR_LOG("usleep failed");
	    thread_func_args->thread_complete_success = false;
	    return thread_param;
    	}
	
    	retVal = pthread_mutex_unlock(thread_func_args->mutex_thread);	//unlock mutex
	
	if(retVal !=0 )
	{
	    ERROR_LOG("pthread_mutex_unlock failed");
	    thread_func_args->thread_complete_success = false;
	    return thread_param;
    	}
   
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     
    int retVal;
    struct thread_data *threadData;
    
    threadData = (struct thread_data *) malloc(sizeof (struct thread_data));
    
    if (threadData == NULL)	//check for memory allocation error
    {
    	ERROR_LOG("Thread memory allocation failed\n");
    }
    DEBUG_LOG("Thread memory allocation failed successfull\n");
    
   // assign the struct parameters
   threadData->wait_to_obtain_ms = wait_to_obtain_ms;
   threadData->wait_to_release_ms = wait_to_release_ms;
   threadData->mutex_thread = mutex;
  
   retVal = pthread_create(thread, NULL, &threadfunc, (void *)threadData);	//create thread
   
   if (retVal != 0)
   {
       ERROR_LOG("Thread creation failed with return value = %d\n", retVal);
       return false;
   }
   
   DEBUG_LOG("Thread creation successfull\n");
    
   return true;
}

