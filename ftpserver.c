/*
 * File name: ftpserver.c
 * Project: FTP TCP Assignment 2
 * CPSC 333
 * Author: @Rakan AlZagha
 * Comments: get, put, and bye all work if ran individually (or get -> bye / put -> bye)
 * Main issue is program is unable to take continous input without erroring at write for second command (just for get/put, bye and get/put works)
 * Need to run each command in different FTP client sessions, file transfers successfully with one server session open
 */

#define SERVER_CONTROL_PORT 9001
#define SERVER_DATA_PORT 9000
#define BUFFERSIZE 8192

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
#include <signal.h>

void handle_ctrlc(int sig);
int getInput(char* command, const char* possibilities[]);
int commandCheck(char* command, const char* possibilities[]);

int accept_socket;

/*
 *   Function: main()
 *   ----------------------------
 *   Purpose: wait for PASV TCP/FTP connections
 *   with a client to transfer files/data
 *
 */

int main( int argc, char *argv[] ) {
    struct sockaddr_in server_control_address;
    struct sockaddr_in server_data_address;
    struct sockaddr_in client_address; //socket address for client
    struct hostent *hostConvert;       //convert hostname to IP
    struct in_addr inaddr;             //internet address
    char* ip;
    char* hostname;
    char data_buffer[BUFFERSIZE];            //buffer of size 100
    int control_socket, data_socket, n, file, bind_override = 1, address_size, error_check, validCommand;
    const char* possibleCommands[3] = {"get", "put", "bye"}; //array for command possibilites
    bool isValidState = true;
    char command[50], filename[50];

    address_size = sizeof(client_address); //establishing the size of the address

    signal(SIGINT, handle_ctrlc);

    //socket structures set to null
    bzero(&server_data_address, sizeof(server_data_address));
    bzero(&client_address, sizeof(client_address));
    bzero(&server_control_address, sizeof(server_control_address));

    //control_socket implementation
    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(control_socket < 0){ //error checking
        perror("Failed to create socket, please try again!");
        exit(1);
    }
    if (setsockopt(control_socket, SOL_SOCKET, SO_REUSEADDR, &bind_override, sizeof(int)) < 0) //bind kept failing after repeated use, SO_REUSEADDR fixed the problem
      error("setsockopt(SO_REUSEADDR) failed");

    //server ipv4, server port, and IP address
    server_control_address.sin_family = AF_INET;
    server_control_address.sin_port = htons(SERVER_CONTROL_PORT);
    server_control_address.sin_addr.s_addr = htonl(INADDR_ANY);

    //assign the address specified by addr to the socket referred to by the file descriptor
    error_check = bind(control_socket, (struct sockaddr *)&server_control_address, sizeof(server_control_address));
    if(error_check < 0){ //error checking
        perror("ERROR at bind function!");
        exit(1);
    }

    error_check = listen(control_socket, 1);
        if(error_check < 0){ //error checking
            perror("ERROR at listen function!");
            exit(1);
        }

    //data_socket implementation
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(data_socket < 0){ //error checking
        perror("Failed to create socket, please try again!");
  exit(1);
    }
    if (setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &bind_override, sizeof(int)) < 0) //bind kept failing after repeated use, SO_REUSEADDR fixed the problem
      error("setsockopt(SO_REUSEADDR) failed");;

    //server ipv4, server port, and IP address
    server_data_address.sin_family = AF_INET;
    server_data_address.sin_port = htons(SERVER_DATA_PORT);
    server_data_address.sin_addr.s_addr = htonl(INADDR_ANY);

    //assign the address specified by addr to the socket referred to by the file descriptor
    error_check = bind(data_socket, (struct sockaddr *)&server_data_address, sizeof(server_data_address));
    if(error_check < 0){ //error checking
        perror("ERROR at bind function!");
        exit(1);
    }

    error_check = listen(data_socket, 1);
    if(error_check < 0){ //error checking
        perror("ERROR at listen function!");
        exit(1);
    }

    accept_socket = accept(control_socket, (struct sockaddr *)&client_address, &address_size); //accept connection request
    data_socket = accept(data_socket, (struct sockaddr *)&client_address, &address_size); //accept connection request

    while(isValidState){ //continue taking input
        bzero(command, sizeof(command));
        read(accept_socket, command, sizeof(command));
        validCommand = getInput(command, possibleCommands);
        if(validCommand == -1) { //if command is not recognizable from possible commands array
            printf("???\n"); //error message
        }
        else { //if command is recognized, pass it into the switch cases to call correct function
            switch(validCommand) { //based on possibleCommands array
                case 0: //get
                    bzero(filename, sizeof(filename));
                    read(accept_socket, filename, sizeof(filename)); //read in filename
                    printf("Command Successful...\n");
                    file = open(filename, O_RDONLY, S_IRUSR); //open file
                    while((n = read(file, data_buffer, BUFFERSIZE)) > 0){ //read data in
                        data_buffer[n] = '\0'; //set buffer to null
                        if(write(data_socket, data_buffer, n) < n){ //send it to the client
                            perror("ERROR in writing\n");
                            exit(1);
                        }
                    }
                    printf("Transfer complete. (get)\n");
                    close(file);
                break;

                case 1:{ //put
                    bzero(filename, sizeof(filename));
                    read(accept_socket, filename, sizeof(filename)); //read in filename
                    printf("%s\n", filename);
                    printf("Command Successful...\n");
                    file = open(filename, O_WRONLY|O_CREAT, S_IRWXU); //open file
                    while((n = read(data_socket, data_buffer, BUFFERSIZE)) > 0){ //read data in
                        data_buffer[n] = '\0'; //set buffer to null
                        if(write(file, data_buffer, n) < n){ //copy data into file from client
                                perror("ERROR in writing\n");
                                exit(1);
                        }
                    }
                    printf("Transfer complete. (put)\n");
                    close(file);
                break;
                }

                case 3: { //bye
                    isValidState = 0;
                    close(data_socket);
                    close(control_socket);
                    exit(0); //client session is now terminated, FTP ends
                break;
                }
            }
        }
        continue;
    }
}
  

/*
 *   Function: handle_ctrlc()
 *   ----------------------------
 *   Purpose: handle ctrl-c command
 *
 *   int sig
 *
 */

void handle_ctrlc(int sig) {
    fprintf(stderr, "\nCtrl-C pressed\n");
    close(accept_socket);
    exit(0);
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
