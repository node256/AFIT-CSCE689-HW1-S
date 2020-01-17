#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "TCPClient.h"
#include "strfuncts.h"


/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
 **********************************************************************************************/

TCPClient::TCPClient() {
}

/**********************************************************************************************
 * TCPClient (destructor) - No cleanup right now
 *
 **********************************************************************************************/

TCPClient::~TCPClient() {

}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
 **********************************************************************************************/
// Brian "Beej" Hall https://beej.us/guide/bgnet/html/#connect
void TCPClient::connectTo(const char *ip_addr, unsigned short port) {

    // convert port to const char* for getaddrinfo()
    const char *PORT = ustcchp(port);

    int rv;
    struct addrinfo hints, *res;

    // set up socket address/port
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // verify address/port
    if ((rv = getaddrinfo(ip_addr, PORT, &hints, &res)) != 0){
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    // make and verfy socket
    this->_clientSockFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (this->_clientSockFD == -1) { 
        perror("socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    } 

    // appempt to connect and verify
    if (connect(this->_clientSockFD, res->ai_addr, res->ai_addrlen) < 0 ){
        perror("connect failed\n");
        closeConn();
        exit(EXIT_FAILURE);
    }

}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() {
    char buff[socket_bufsize]; 
    int n;

    // request menu
    bzero(this->_buff, socket_bufsize);
    int nbytes = recv(this->_clientSockFD, this->_buff, socket_bufsize, 0);
    sleep(1);
    std::cout << this->_buff;

    for (;;) { 
        bzero(buff, sizeof(buff)); 
        n = 0; 
        while ((buff[n++] = getchar()) != '\n') ; 
        write(this->_clientSockFD, buff, sizeof(buff)); 
        bzero(buff, sizeof(buff)); 
        read(this->_clientSockFD, buff, sizeof(buff)); 
        if ((strncmp(buff, "exit", 4)) == 0) {  
            break; 
        } 
        printf("%s", buff); 
        
    } 
}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() {
    close(this->_clientSockFD);
}


