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

/******************MACROS******************************/
#define FILE_STORAGE_PATH   "/var/tmp/aesdsocketdata"
#define PORT "9000"
#define SOCKET_FAMILY     	AF_INET6
#define SOCKET_TYPE       	SOCK_STREAM
#define SOCKET_FLAGS      	AI_PASSIVE
#define FILE_PERMISSIONS  	0644
#define BUFFER_SIZE       	1024
#define MAX_CONNECTIONS   	10

/******************GLOBAL VARIABLES******************************/
char *output_buffer = NULL;
int socket_fd = 0; //socket file descriptor
int socket_accept_fd = 0;
char *port = "9000";

/***********************************************************************************************
 * Function Name          : signal_handler
 * Description        	  : To capture a singnal and take respective actons as per the signal.
 * Input Parameters       : sig_no i.e the signal received that we need to handle
 * Returns                : None
 ***********************************************************************************************/
static void signal_handler (int signo)
{
  if(signo == SIGINT)
    {
      printf("Caught SIGINT signal, exiting\n");
      unlink(FILE_STORAGE_PATH); //delete the file
      free(output_buffer);
    }
  else if(signo == SIGTERM)
    {
      printf("Caught SIGTERM signal, exiting\n");
      unlink(FILE_STORAGE_PATH); //delete the file
      free(output_buffer);
    }
  else if(signo == SIGKILL)
    {
      printf("Caught SIGKILL signal, exiting\n");
      unlink(FILE_STORAGE_PATH); //delete the file
      free(output_buffer);
      close(socket_accept_fd);
      close(socket_fd);
    }

  exit(EXIT_SUCCESS);
}


/***********************************************************************************************
 * Name            : handle_socket
 * Description     : The server socket configuration routines are called
 * Input Parameters  : None
 * Returns         : None
 ***********************************************************************************************/
static void handle_socket()
{
  struct addrinfo hints;
  struct addrinfo *result_ptr;  
  struct sockaddr_in clientaddress;
  socklen_t client_sock_addr_size = sizeof(struct sockaddr); 
  char buffer[BUFFER_SIZE] = {0};

  int addr_info;
  int socket_bind = 0; 
  int received_packet_size = 0;
  int file_fd = 0;
  int socket_listen = 0; 
  int file_write;
  int read_file_data;
  int send_data;
  int i;
  int recvive_fd = 0;

  bool is_packet_received = false;

  char ch = 0;
  char ipv_4[INET_ADDRSTRLEN];

  memset(buffer,0,BUFFER_SIZE);

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
      syslog(LOG_INFO,"Error: File creation succeed");
    }

  close(file_fd); //close fd after creating

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

  while(1)
    {
      output_buffer = (char *) malloc((sizeof(char)*BUFFER_SIZE));
      if(output_buffer == NULL)
        {
          printf("Error! Malloc has failed\n");
          syslog(LOG_ERR, "Malloc has failed\n");
          exit(1);
        }
      else
        {
          printf("Malloc succeed\n");
          syslog(LOG_INFO, "Malloc succeed\n");
        }
      memset(output_buffer,0,BUFFER_SIZE);


      //6. Listen on the socket
	  

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

      //8. Receive a packet
	is_packet_received = false;
      while(!is_packet_received) 
        {
          recvive_fd = recv(socket_accept_fd, buffer, BUFFER_SIZE ,0);
          //printf("buffer1 = %s \n",buffer);
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
	printf("Output buffer = %s \n",output_buffer);
	printf("Length = %ld \n",strlen(output_buffer));
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
          send_data  = send(socket_accept_fd,&ch,1,0);  //send
          if(send_data == -1)
            {
              printf("Error in sending file data\n");
              syslog(LOG_ERR,"Error in sending file data");
              exit(1);
            }
            //printf("Send succeeded\n");
        }
      close(file_fd);
      free(output_buffer); // free the buffer
      //close(socket_fd);
      //close(socket_accept_fd);
  }
      //11. Close the  socket_fd and socket_accept_fd
	  

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
