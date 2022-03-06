/***********************************************************************************************************************
 * Author       : Amey Sharad Dashaputre
 * File Name    : aesdsocket.c
 * Description  : Socket Functions implementation
 ***********************************************************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/queue.h>
#include <sys/time.h>

/******************MACROS******************************/
#define FILE_STORAGE_PATH   "/var/tmp/aesdsocketdata"
#define PORT "9000"
#define SOCKET_FAMILY       AF_INET6
#define SOCKET_TYPE         SOCK_STREAM
#define SOCKET_FLAGS        AI_PASSIVE
#define FILE_PERMISSIONS    0644
#define BUFFER_SIZE         1024
#define MAX_CONNECTIONS     10

/******************GLOBAL VARIABLES******************************/
//char *output_buffer = NULL;
int socket_fd = 0; //socket file descriptor
int socket_accept_fd = 0;
char *port = "9000";
int file_fd = 0;
int received_packet_size = 0;
char ch = 0;

void* thread_handler(void* thread_param);

typedef struct
{
  bool thread_comp_flag;
  pthread_t thread_id;
  int cl_accept_fd;
  pthread_mutex_t *mutex;
}thread_params;

//Linked list node
struct slist_data_s
{
  thread_params thread_param;
  SLIST_ENTRY(slist_data_s) entries;
};

typedef struct slist_data_s slist_data_t;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
slist_data_t *datap = NULL;
SLIST_HEAD(slisthead, slist_data_s) head;

/***********************************************************************************************
 * Name            : timer_handler
 * Description     : used to catch the SIGALRM signal after the timer expiry.
 * Input Parameters  : sig_no - the signal received
 * Returns         : None
 ***********************************************************************************************/
static void timer_handler(int sig_no)
{
  char time_stamp[200];                         
  time_t t;
  struct tm *time_ptr;
  int timer_length = 0;
  int mutex_return_val;
  int write_return_val;

  t = time(NULL);
  time_ptr = localtime(&t);
  if(time_ptr == NULL)
    {
      syslog(LOG_ERR, "localtime() function failed\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      syslog(LOG_INFO, "localtime() function succeed\n");
    }

  timer_length = strftime(time_stamp,sizeof(time_stamp),"timestamp:%d.%b.%y - %k:%M:%S\n",time_ptr);
  if(timer_length == 0)
    {
      syslog(LOG_ERR, "strftime() function failed\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      syslog(LOG_INFO, "strftime() function succeed\n");
    }

  printf("time value:%s\n",time_stamp); //Print the time

  file_fd = open(FILE_STORAGE_PATH,O_APPEND | O_WRONLY); //Write the time in file
  if(file_fd == -1)
    {
      syslog(LOG_ERR, "Error while file open for appending\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      syslog(LOG_INFO, "Successfully appened the time data in the file\n");
    }

  mutex_return_val = pthread_mutex_lock(&mutex_lock);
  if(mutex_return_val != 0)
    {
      syslog(LOG_ERR, "pthread_mutex_lock() failed \n");
      exit(EXIT_FAILURE);
    }

  write_return_val = write(file_fd,time_stamp,timer_length);
  if (write_return_val == -1)
    {
      syslog(LOG_ERR, "write() to file failed\n");
      exit(EXIT_FAILURE);
    }
  else if (write_return_val != timer_length)
    {
      syslog(LOG_ERR, "The file partially written\n");
      exit(EXIT_FAILURE);
    }
  received_packet_size += timer_length; // incrremnet the received_packet_size global variable

  mutex_return_val = pthread_mutex_unlock(&mutex_lock);
  if(mutex_return_val != 0)
    {
      syslog(LOG_ERR, "pthread_mutex_unlock() failed \n");
      exit(EXIT_FAILURE);
    }

  close(file_fd);
}

/***********************************************************************************************
 * Function Name          : signal_handler
 * Description            : To capture a singnal and take respective actons as per the signal.
 * Input Parameters       : sig_no i.e the signal received that we need to handle
 * Returns                : None
 ***********************************************************************************************/
static void signal_handler (int signo)
{
  if(signo == SIGINT)
    {
      printf("Caught SIGINT signal, exiting\n");
      unlink(FILE_STORAGE_PATH); //delete the file
      //free(output_buffer);
    }
  else if(signo == SIGTERM)
    {
      printf("Caught SIGTERM signal, exiting\n");
      unlink(FILE_STORAGE_PATH); //delete the file
      //free(output_buffer);
    }
  else if(signo == SIGKILL)
    {
      printf("Caught SIGKILL signal, exiting\n");
      while(SLIST_FIRST(&head) != NULL)
      {
	SLIST_FOREACH(datap,&head,entries)
	{
		close(datap->thread_param.cl_accept_fd);
		pthread_join(datap->thread_param.thread_id,NULL);
		SLIST_REMOVE(&head, datap, slist_data_s, entries);
		free(datap);
		break;
	}
     }
	pthread_mutex_destroy(&mutex_lock); //Free mutex
       unlink(FILE_STORAGE_PATH); //delete the file
      //free(output_buffer);
      close(socket_accept_fd);
      close(socket_fd);
    }
  exit(EXIT_SUCCESS);
}

/***********************************************************************************************
 * Name            : handle_socket
 * Description     : The server socket configuration routines are called
 * Input Parameters: None
 * Returns         : None
 ***********************************************************************************************/
static void handle_socket()
{
  struct addrinfo hints;
  struct addrinfo *result_ptr;  
  struct sockaddr_in clientaddress;
  socklen_t client_sock_addr_size = sizeof(struct sockaddr); 

  int addr_info;
  int socket_bind = 0; 
  int socket_listen = 0; 
  int sockopt_ret = 0;
  int timer_ret = 0;

  //SLIST_HEAD(slisthead, slist_data_s) head;
  SLIST_INIT(&head);

  struct itimerval interval_timer;
  char ipv_4[INET_ADDRSTRLEN];

  //1. clear the hints first

  memset(&hints,0,sizeof(hints));
  hints.ai_family = SOCKET_FAMILY;
  hints.ai_socktype = SOCKET_TYPE;
  hints.ai_flags = SOCKET_FLAGS;

  //2. set the sockaddr using getaddrinfo

  addr_info = getaddrinfo(NULL,port,&hints,&result_ptr);
  if(addr_info != 0)
    {
      printf("Error!! getaddrinfo() failed\n");
      syslog(LOG_ERR,"Error: getaddrinfo failed");
      exit(1);
    }
  else
    {
      printf("getaddrinfo() succeed \n");
      syslog(LOG_INFO,"getaddrinfo() succeed ");
    }

  //3.create a socket

  socket_fd = socket(SOCKET_FAMILY, SOCK_STREAM, 0);//IPv4,TCP
  if(socket_fd == -1)
    {
      printf("Error! socket() creation failed\n");
      syslog(LOG_ERR,"socket() creation failed");
      freeaddrinfo(result_ptr);
      exit(1);
    }
  else
    {
      printf("socket() creation succeed \n");
      syslog(LOG_INFO,"socket() creation succeed ");
    }

  // setsockopt setting 
  sockopt_ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
  if(sockopt_ret < 0)
    {
      printf("Error! setsockopt() setting failed\n");
      syslog(LOG_ERR,"setsockopt() setting failed");
      freeaddrinfo(result_ptr);
      exit(1);
    }
  else
    {
      printf("setsockopt() setting succeed \n");
      syslog(LOG_INFO,"setsockopt() setting  succeed ");
    }

  //4. Bind the socket

  socket_bind = bind(socket_fd, result_ptr->ai_addr, result_ptr->ai_addrlen);
  if(socket_bind == -1)
    {
      printf("Error! bind() socket failed\n");
      syslog(LOG_ERR,"Error: bind() socket failed");
      freeaddrinfo(result_ptr);
      exit(1);
    }
  else
    {
      printf("bind() socket succeed\n");
      syslog(LOG_INFO,"bind() socket succeed");
    }

  freeaddrinfo(result_ptr); //free after use
  //5. create a file

  file_fd = creat(FILE_STORAGE_PATH, FILE_PERMISSIONS);
  if(file_fd == -1)
    {
      printf("Error! File creation failed\n");
      syslog(LOG_ERR,"Error: File creation failed");
      exit(1);
    }
  else
    {
      printf("File creation succeed\n");
      syslog(LOG_INFO,"File creation succeed");
    }

  close(file_fd); //close fd after creating

  interval_timer.it_interval.tv_sec = 10; //timer interval of 10 secs
  interval_timer.it_interval.tv_usec = 0;
  interval_timer.it_value.tv_sec = 10; //time expiration of 10 secs
  interval_timer.it_value.tv_usec = 0;

  timer_ret = setitimer(ITIMER_REAL, &interval_timer, NULL);
  if(timer_ret == -1)
    {
      printf("setitimer() function failed");
      syslog(LOG_ERR, "setitimer() function failed");
    }
  else
    {
      printf("setitimer() function succeed\n");
      syslog(LOG_INFO,"setitimer() function succeed");
    }

  while(1)
    {

      //6. Listen on the socket
      socket_listen = listen(socket_fd, MAX_CONNECTIONS);
      if(socket_listen == -1)
        {
          printf("Error! Socket Listen failed\n");
          syslog(LOG_ERR,"Socket Listen failed");
          freeaddrinfo(result_ptr);
          exit(1);
        }
      else
        {
          printf("Socket Listen succeed\n");
          syslog(LOG_INFO,"Socket Listen succeed");
        }

      //7. Accept the socket  

      socket_accept_fd = accept(socket_fd,(struct sockaddr *)&clientaddress, &client_sock_addr_size);
      if(socket_accept_fd == -1)
        {
          printf("Error: accept() socket failed\n");
          syslog(LOG_ERR,"accept() socket failed");
          freeaddrinfo(result_ptr);
          exit(1);
        }
      else
        {
          printf("accept() socket succeed\n");
          syslog(LOG_INFO,"accept() socket succeed");
        }

      //get ip address
      struct sockaddr_in *addr = (struct sockaddr_in *)&clientaddress;

      inet_ntop(AF_INET, &(addr->sin_addr),ipv_4,INET_ADDRSTRLEN);
      syslog(LOG_DEBUG,"Accepted the connection from %s",ipv_4);
      printf("Accepted the connection from %s\n",ipv_4);

      //allocating new node for the data
      datap = (slist_data_t *) malloc(sizeof(slist_data_t));
      SLIST_INSERT_HEAD(&head,datap,entries);

      //Inserting thread parameters now
      datap->thread_param.cl_accept_fd = socket_accept_fd;
      datap->thread_param.thread_comp_flag = false;
      datap->thread_param.mutex = &mutex_lock;

      pthread_create(&(datap->thread_param.thread_id),       //the thread id to be created
                     NULL,     //the thread attribute to be passed
                     thread_handler,       //the thread handler to be executed
                     &datap->thread_param//the thread parameter to be passed
      );    

      printf("All thread that are created are now waiting to exit\n");

      SLIST_FOREACH(datap,&head,entries)
      {
        
        if(datap->thread_param.thread_comp_flag == true)
          {
            pthread_join(datap->thread_param.thread_id,NULL);
            SLIST_REMOVE(&head, datap, slist_data_s, entries);
            free(datap);
            break;
          }
      }  
      printf("All threads are exited!\n");
      syslog(LOG_DEBUG,"Closed connection from %s",ipv_4);
      printf("Closed connection from %s\n",ipv_4);

    }
  close(socket_accept_fd);
  close(socket_fd);
}

/***********************************************************************************************
 * Name          : thread_handler
 * Description   : performs the thread specific client data read, write to file and send steps. 
 * Parameters    : thread parameter
 * RETURN        : thread parameter
 ***********************************************************************************************/
void* thread_handler(void* thread_param)
{
  int i;
  int mutex_return_val = 0;
  int file_write;
  int read_file_data;
  int recvive_fd = 0;
  int file_fd =0;
  int send_data =0;

  char buffer[BUFFER_SIZE] = {0};
  char *output_buffer  = NULL;

  bool is_packet_received;

  thread_params * params = (thread_params *) thread_param;

  output_buffer = (char *) malloc((sizeof(char)*BUFFER_SIZE));

  if(output_buffer == NULL)
    {
      syslog(LOG_ERR, "Malloc failed\n");
      printf("Malloc failed\n");
      exit(1);
    }
  else
    {
      printf("accept() Malloc succeed\n");
      syslog(LOG_INFO,"Malloc() socket succeed");
    }
  memset(output_buffer,0,BUFFER_SIZE);

  //8. Receive a packet
  is_packet_received = false;
  while(!is_packet_received) 
    {
      recvive_fd = recv(params->cl_accept_fd, buffer, BUFFER_SIZE ,0);

      if(recvive_fd < 0)
        {
          printf("Error: Receive packet failed\n");
          syslog(LOG_ERR,"Receive packet failed");
          exit(1);
        }
      else
        {
          printf("Receive packet succeed\n");
          syslog(LOG_INFO,"Receive packet succeed");
        }

      for(i = 0;i < BUFFER_SIZE;i++)
        { 
          if(buffer[i] == '\n')  // check at end of packet
            {
              is_packet_received = true;    //end of packet detected
              i++;          
              break;
            }
        }
      received_packet_size += i; //increment the size of packet
      output_buffer = (char *) realloc(output_buffer,(received_packet_size+1)); //reloaction to a buffer big in size
      if(output_buffer == NULL)
        {
          printf("Realloc() has failed\n");
          syslog(LOG_ERR, "Realloc has failed!\n");
          exit(1);
        }
      else
        {
          printf("Realloc() succeed\n");
          syslog(LOG_INFO, "Realloc succeed\n");
        }
      //printf("buffer = %s \n",buffer);
      strncat(output_buffer,buffer,i);
      memset(buffer,0,BUFFER_SIZE); 
    }

  mutex_return_val = pthread_mutex_lock(params->mutex);
  if (mutex_return_val != 0)
    {
      printf("pthread_mutex_lock() has failed\n");
      syslog(LOG_ERR, "pthread_mutex_lock() has failed\n");
      exit(EXIT_FAILURE);
    }

  //9. start writing to the file by creating the file in append mode

  file_fd = open(FILE_STORAGE_PATH,O_APPEND | O_WRONLY); // open file in append mode and write the data received from the client in it
  if(file_fd == -1)
    {
      printf("File open error whilvalgrinde appending\n");
      syslog(LOG_ERR, "File open error while appending");
      exit(1);
    }
  else
    {
      printf("File open succeed while appending\n");
      syslog(LOG_INFO, "File open succeed while appending");
    }

  file_write = write(file_fd,output_buffer,strlen(output_buffer));
  if(file_write == -1)
    {
      printf("Error! File could not be written\n");
      syslog(LOG_ERR,"File could not be written!");
      exit(1);
    }
  else if(file_write != strlen(output_buffer))
    {
      printf("Error! File is partially written\n");
      syslog(LOG_ERR,"File is partially written");
      exit(1);
    }
  else
    {
      printf("File is written successfully\n");
      syslog(LOG_INFO,"File is written successfully");
    }

  printf("Writing data to file %s \n",output_buffer);

  close(file_fd);

  memset(buffer,0,BUFFER_SIZE);
  file_fd = open(FILE_STORAGE_PATH,O_RDONLY); //open the file in readonly mode
  if(file_fd == -1)
    {
      printf("Error while file reading\n");
      syslog(LOG_ERR,"Error while file reading");
      exit(1);
    }
  else
    {
      printf("File reading succeed\n");
      syslog(LOG_ERR,"File reading succeed");
    } 

  //10. Sending data 

  for(i=0;i<received_packet_size;i++)
    {
      read_file_data = read(file_fd, &ch, 1); //read the file data
      if(read_file_data == -1)
        {
          printf("File Read failed\n");
          syslog(LOG_ERR,"File Read failed");
          exit(1);
        }
      else
        {
          //printf("File Read succeed\n");
          syslog(LOG_INFO,"File Read succeed");
        }
      send_data  = send(params->cl_accept_fd,&ch,1,0);  //send
      if(send_data == -1)
        {
          printf("Error in sending file data\n");
          syslog(LOG_ERR,"Error in sending file data");
          exit(1);
        }
      //printf("Send succeeded\n");
    }
  close(file_fd);

  mutex_return_val = pthread_mutex_unlock(params->mutex);
  if(mutex_return_val != 0)
    {
      printf("pthread_mutex_unlock() has failed\n");
      syslog(LOG_ERR,"pthread_mutex_unlock() has failed");
      exit(1);
    }

  params->thread_comp_flag = true;

  close(params->cl_accept_fd);

  free(output_buffer); // free the buffer

  return params;
}  

/***********************************************************************************************
 * Name             : main
 * Description      : Appliction entry point
 * Input Parameters : argc and argv[] 
 * Returns          : The exit status
 ***********************************************************************************************/
int main(int argc, char* argv[])
{
  int daemon_ret = 0;
  openlog("aesdsocket-a5",LOG_PID,LOG_USER); // open the log file

  if(signal(SIGINT, signal_handler)==SIG_ERR)
    {
      printf("Failed to configure SIGINT handler\n");
      syslog(LOG_ERR, "Failed to configure SIGINT handler\n");
      exit(1);
    }
  if(signal(SIGTERM, signal_handler)==SIG_ERR)
    {
      printf("Failed to configure SIGTERM handler\n");
      syslog(LOG_ERR, "Failed to configure SIGTERM handler\\n");
      exit(EXIT_FAILURE);
    }
  if(signal(SIGALRM, timer_handler)==SIG_ERR)
    {
      printf("Failed to configure SIGALRM handler\n");
      syslog(LOG_ERR, "Failed to configure SIGALRM handler\n");
      exit(EXIT_FAILURE);
    }

  pthread_mutex_init(&mutex_lock, NULL);

  signal(SIGALRM,timer_handler); //Timer configutaion for A6-P1

  if(argc > 1) 
    {
      if((strcmp("-d",(char*)argv[1])) == 0)
        {
          printf("Entering the daemon mode\n");
          syslog(LOG_DEBUG,"Entering the daemon mode");
          daemon_ret = daemon(0,0);
          if(daemon_ret == -1)
            {
              printf("Entering daemon mode failed\n");
              syslog(LOG_DEBUG,"Entering daemon mode failed");
            }
        }
    }
  handle_socket();
  closelog();
  return 0;
}
