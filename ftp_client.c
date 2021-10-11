/*
 * File name: ftpclient.c
 * Project: FTP TCP Assignment 2
 * CPSC 333
 * Author: @Rakan AlZagha
 * Comments: get, put, and bye all work if ran individually (or get -> bye / put -> bye)
 * Main issue is program is unable to take continous input without erroring at write for second command (just for get/put, bye and get/put works)
 * Need to run each command in different FTP client sessions, file transfers successfully with one server session open
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_CONTROL_PORT 9001
#define SERVER_DATA_PORT 9000
#define BUFFERSIZE 8192

int getInput(char* command, const char* possibilities[]);
int commandCheck(char* command, const char* possibilities[]);
void get(int control_socket, int data_socket, char* filename, char data_buffer[BUFFERSIZE]);
void put(int control_socket, int data_socket, char* filename, char data_buffer[BUFFERSIZE]);
bool bye(bool isValidState);

/*
 *   Function: main()
 *   ----------------------------
 *   Purpose: establish a PASV TCP/FTP connections
 *   with a server to transfer files/data
 *
 */

int main( int argc, char *argv[] ) {
    struct sockaddr_in server_control_address;
    struct sockaddr_in server_data_address;
    struct hostent *hostConvert;       //convert hostname to IP
    struct in_addr inaddr;             //internet address
    char* ip;
    char* hostname;.
    int control_socket, bind_override = 1, data_socket, n, file, control_address_size, data_address_size, error_check, validCommand;
    const char* possibleCommands[3] = {"get", "put", "bye"}; //array of possible commands
    bool isValidState = true;
    char command[50], filename[50], data_buffer[BUFFERSIZE];

    hostname = argv[1]; //extracting the name of the host we are connecting to (first argument)
    hostConvert = gethostbyname(hostname); //converting name to host address
    if(hostConvert == NULL){ //error checking that host is valid (ex. syslab001... would error)
        printf("Not a valid hostname. Please try again.\n");
        exit(1);
    }
    ip = inet_ntoa(*(struct in_addr *) *hostConvert->h_addr_list); //converts host address to a string in dot notation

    //control_socket declaration
    bzero(&server_control_address, sizeof(server_control_address)); //socket structures set to null
    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(control_socket < 0){ //error checking
        perror("Failed to create control socket, please try again!");
        exit(1);
    }
    if (setsockopt(control_socket, SOL_SOCKET, SO_REUSEADDR, &bind_override, sizeof(int)) < 0) //bind kept failing after repeated use, SO_REUSEADDR fixed the problem
      error("setsockopt(SO_REUSEADDR) failed");

    //server ipv4, server port, and IP address
    server_control_address.sin_family = AF_INET;
    server_control_address.sin_port = htons(SERVER_CONTROL_PORT);
    server_control_address.sin_addr.s_addr = inet_addr(ip);

    control_address_size = sizeof(server_control_address); //establishing the size of the address
    data_address_size = sizeof(server_control_address); //establishing the size of the address

    error_check = connect(control_socket, (struct sockaddr *)&server_control_address, control_address_size);
    if (error_check != 0){ //error checking
        perror("ERROR in connect function");
        exit(1);
    }

    //data_socket declaration
    bzero(&server_data_address, sizeof(server_data_address));
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(data_socket < 0){ //error checking
        perror("Failed to create data socket, please try again!");
        exit(1);
    }
    if (setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &bind_override, sizeof(int)) < 0) //bind kept failing after repeated use, SO_REUSEADDR fixed the problem
      error("setsockopt(SO_REUSEADDR) failed");
    
    //server ipv4, server port, and IP address
    server_data_address.sin_family = AF_INET;
    server_data_address.sin_port = htons(SERVER_DATA_PORT);
    server_data_address.sin_addr.s_addr = inet_addr(ip);

    error_check = connect(data_socket, (struct sockaddr *)&server_data_address , data_address_size);
    if (error_check != 0){ //error checking
        perror("ERROR in connect function");
        exit(1);
    }

   while(isValidState){ //continue taking input until bye command is executed
        printf("ftp>");
        scanf("%s", command);
        write(control_socket, command, 50);
        validCommand = getInput(command, possibleCommands);
        if(validCommand == -1) { //if command is not recognizable from possible commands array
            printf("???\n"); //error message
        }
        else { //if command is recognized, pass it into the switch cases to call correct function
            switch(validCommand) { //based on possibleCommands array
                case 0: //get command
                  scanf("%s", filename);
                  get(control_socket, data_socket, filename, data_buffer);
                break;

                case 1: //put command
                  scanf("%s", filename);
                  put(control_socket, data_socket, filename, data_buffer);
                break;

                case 2: //bye command
                  isValidState = bye(isValidState);
                break;
            }
        }
      close(control_socket);
      close(data_socket);
      continue;
    }
}
      
/*
 *   Function: get()
 *   ----------------------------
 *   Purpose: read/write data into a local file from a remote server
 *
 *   int control_socket
 *   int data_socket
 *   char* filename
 *   char data_buffer[BUFFERSIZE]
 *
 */

void get(int control_socket, int data_socket, char* filename, char data_buffer[BUFFERSIZE]){
  int file, n;
  write(control_socket, filename, 50); //send filename to server
  printf("Command Sent Successfully...\n");
  file = open(filename, O_WRONLY|O_CREAT, S_IRWXU); //create and or write privelages
  while((n = read(data_socket, data_buffer, BUFFERSIZE)) > 0){ //read data in
    data_buffer[n] = '\0';
    if(write(file, data_buffer, n) < n){ //write to the file
      perror("ERROR in writing\n");
      exit(1);
    }
    break;
  }
  printf("Transfer complete. (get)\n");
  close(file);
}

/*
 *   Function: put()
 *   ----------------------------
 *   Purpose: read/write from local file to a remote server
 *
 *   int control_socket
 *   int data_socket
 *   char* filename
 *   char data_buffer[BUFFERSIZE]
 *
 */

void put(int control_socket, int data_socket, char* filename, char data_buffer[BUFFERSIZE]){
  int file, n;
  write(control_socket, filename, 50); //send filename over to the server
  printf("Command Sent Successfully...\n");
  if((file = open(filename, O_RDONLY, S_IRUSR)) < 0){ //open local file to read
    exit(1);
  }
   while((n = read(file, data_buffer, BUFFERSIZE)) > 0){ //read local file
    data_buffer[n] = '\0'; //set buffer to NULL prior to writing
    if(write(data_socket, data_buffer, n) < n){ //write date over to the server
      perror("ERROR in writing\n");
      exit(1);
    }
  }
  printf("Transfer complete. (put)\n");
  close(file); //close the file that was opened
}

/*
 *   Function: bye()
 *   ----------------------------
 *   Purpose: terminate the client FTP session
 *
 *   bool isValidState
 *
 */

bool bye(bool isValidState){
  isValidState = 0;
  return(isValidState);
}

/*
 *   Function: getInput()
 *   ----------------------------
 *   Purpose: receive a command from command line and return correct command index
 *
 *   char* command
 *   const char* possibilities[]
 *
 */

int getInput(char* command, const char* possibilities[]) { //
  int validCommand; //position in array will be associated with this int variable
  validCommand = commandCheck(command, possibilities); //check if it is a valid command by cross-referencing possibilities
  return validCommand; //return correct place in the array (correct command)
}

/*
 *   Function: commandCheck()
 *   ----------------------------
 *   Purpose: check if the command is within the realm of possibilities
 *
 *   char* command
 *   const char* possibilities[]
 *
 */

int commandCheck(char* command, const char* possibilities[]) {
  int i, correctCommand = -1; //starts at -1 for error handling case when command DNE
  for(i = 0; i < 3; i++) { //loop through array of commands
    int dataCommand = strcmp(command, possibilities[i]); //check every possibility and store which command is valid
    if(dataCommand == 0) { //if our comparison is successful save position in array
      correctCommand = i;
    }
  }
  if(correctCommand == -1) { //if our loop terminated without changing correctCommand, then command is INVALID
    correctCommand = -1;
  }
  return correctCommand; //return command
}
