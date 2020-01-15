#include "TCPServer.h"
#include <arpa/inet.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sstream>
#include <sys/epoll.h>

TCPServer::TCPServer() {
    
}


TCPServer::~TCPServer() {

}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
// Geeks for Geeks: https://www.geeksforgeeks.org/socket-programming-cc/
// Geeks for Geeks: https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
// Beej's Guide to Network Programming by Brian hall: https://beej.us/guide/bgnet/html/#poll
void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {
    
    // convert port to const char* for getaddrinfo()
    std::stringstream ss;
    ss << port;
    std::string convert = ss.str();
    const char *PORT = convert.c_str();
    
    int optYes = 1;
    int rv;
    struct addrinfo hints, *ai, *p;

    // Seting up server IP address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  
    
    // verify address/port
    if ((rv = getaddrinfo(ip_addr, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Set and verify socket file descriptor
    if (this->_listSockFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol) == 0){
        perror("socket failed\n");
        exit(EXIT_FAILURE);
    }
        
    // prevent "address already in use" error message
    if (setsockopt(this->_listSockFD, SOL_SOCKET, SO_REUSEADDR, &optYes, sizeof(int)) != 0){
        perror("socket opt failed\n");
        exit(EXIT_FAILURE);       
    }

    // set socket to non-blocking, suppoesedly eats CPU cycles
    // poll()/epoll() might make it unecessary according to Beej
    if (fcntl(this->_listSockFD, F_SETFL, O_NONBLOCK) < 0){
        printf("Set non-blocking failed\n");
        exit(EXIT_FAILURE);
    }

    // bind socket file descriptor to IP addr/port
    if (bind(this->_listSockFD, p->ai_addr, p->ai_addrlen) < 0) {
        close(this->_listSockFD);
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }

    // Clear addr info
    freeaddrinfo(ai);

    /*struct sockaddr_in servAddr;
    int opt = 1; 
  
    // socket create and verification 
    this->this->_listSockFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (this->this->_listSockFD == -1) { 
        printf("Socket creation failed...\n"); 
        exit(0); 
    } 
    else printf("Socket successfully created..\n");
    bzero(&servAddr, sizeof(servAddr));

    // setting socket options
    if (setsockopt(this->_listSockFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("Socket options failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket options succeeded..\n");

    // set socket to non-blocking, suppoesedly eats CPU cycles
    if (fcntl(this->_listSockFD, F_SETFL, O_NONBLOCK)){
        printf("Setting non-blocking failed\n");
        exit(0);
    }
    else printf("Setting non-blocking succeeded");
    
    // setting IP address and port
    servAddr.sin_family = AF_INET; 
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_addr.s_addr = inet_addr(ip_addr); 
    servAddr.sin_port = htons(port); 

    // binding IP/port to socket
    if ((bind(this->_listSockFD, (struct sockaddr*)&servAddr, sizeof(servAddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n");
    */
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
// man7.org epoll() man page http://man7.org/linux/man-pages/man7/epoll.7.html
// Beej's Guide to Network Programming by Brian hall: https://beej.us/guide/bgnet/html/#poll
// Linux - IO Multiplexing - Select vs Poll vs Epoll http://man7.org/linux/man-pages/man7/epoll.7.html
void TCPServer::listenSvr() {

    #define MAX_EVENTS 10

    struct sockaddr_storage clientAddr;
    socklen_t addrLen;

    struct epoll_event ev, events[MAX_EVENTS];
    int connSockFD, nfds, epollfd;

    // start and verify listening on socket
    if (listen(this->_listSockFD, 10) < 0 ){
        perror("listen failed\n");
        exit(EXIT_FAILURE);
    }
   
    // Set up epoll file desciptor
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Set up epoll control
    ev.events = EPOLLIN;
    ev.data.fd = this->_listSockFD;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->_listSockFD, &ev) == -1) {
        perror("epoll_ctl: this->_listSockFD");
        exit(EXIT_FAILURE);
    }

    // accepting connections and processing commands
    for (;;){
        // poll file descriptors
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        // process events, accept connections, transfer data
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == this->_listSockFD) {
                connSockFD = accept(this->_listSockFD, (struct sockaddr *) &clientAddr, &addrLen);
                if (connSockFD == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                // set socket to non-blocking, suppoesedly eats CPU cycles
                if (fcntl(connSockFD, F_SETFL, O_NONBLOCK) < 0){
                    printf("Set conn non-blocking failed\n");
                    exit(EXIT_FAILURE);
                }
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connSockFD;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connSockFD,
                            &ev) == -1) {
                    perror("epoll_ctl: connSockFD");
                    exit(EXIT_FAILURE);
                }
            } else {
                //do_use_fd(events[n].data.fd);
            }
        }
    }
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {
    close(this->_listSockFD);
}
