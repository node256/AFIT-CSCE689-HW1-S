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
#include <time.h>
#include "strfuncts.h"

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
// Akshat Shnha https://www.geeksforgeeks.org/socket-programming-cc/
// Yogesh Shukla 1 https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
// Beej's Guide to Network Programming by Brian Hall: https://beej.us/guide/bgnet/html/#poll
void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {
    
    // convert port to const char* for getaddrinfo()
    const char *PORT = ustcchp(port);
    
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

    // create socket and attempt to bind
    for(p = ai; p != NULL; p = p->ai_next) {
        // create socket
        this->_listSockFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (this->_listSockFD < 0) { 
            continue;
        }
        
        // prevent "address already in use" error message
        setsockopt(this->_listSockFD, SOL_SOCKET, SO_REUSEADDR, &optYes, sizeof(int));

        // set listener port to non-blocking
        // Beej says it might eat cpu cycles
        fcntl(this->_listSockFD, F_SETFL, O_NONBLOCK);

        // bind socket
        if (bind(this->_listSockFD, p->ai_addr, p->ai_addrlen) < 0) {
            close(this->_listSockFD);
            continue;
        }

        break;
    }

    // exit if bind failed
    if (p == NULL){
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }

    // Clear addr info
    freeaddrinfo(ai);

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
    #define MAXBUF 256

    struct sockaddr_storage clientAddr;
    socklen_t addrLen;

    struct epoll_event ev, events[MAX_EVENTS];
    int connSockFD, nfds, epollfd;

    char buffer[MAXBUF];
    char * msg;

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

    // Set up epoll control and add listening socket
    ev.events = EPOLLIN;
    ev.data.fd = this->_listSockFD;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->_listSockFD, &ev) == -1) {
        perror("epoll_ctl: this->_listSockFD");
        exit(EXIT_FAILURE);
    }

    // accepting connections and processing commands
    for (;;){
        // poll file descriptors for ready sockets
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        // process events, accept connections, transfer data
        for (int n = 0; n < nfds; ++n) {
            // accept new connections
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
                // add new connection to poll list
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connSockFD;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connSockFD, &ev) == -1) {
                    fprintf(stderr, "epoll set insertion error: fd=%d0", connSockFD);
                    exit(EXIT_FAILURE);
                }

                send(ev.data.fd, this->_menu, strlen(this->_menu), 0);

            // process commands, send/recive data
            } else {
                // zeroize buffer
                bzero(buffer, MAXBUF);

                // read from client to buffer
                int nbytes = recv(events[n].data.fd, buffer, sizeof(buffer), 0);

                // check for closed or errored connections
                int sender_fd = events[n].data.fd;
                if (nbytes <= 0) {
                    if (nbytes == 0) {
                        // Connection closed
                        printf("pollserver: socket %d hung up\n", sender_fd);
                    } else {
                        perror("recv");
                    }

                    // close fd if errored or closed
                    close(events[n].data.fd);

                } 
                else {
                    // process client input and send response
                    /*if ( strncmp(buffer,"hello", 5) ){
                        snprintf(buffer, MAXBUF, "Hello Dave\n");
                    }
                    else if ( strncmp(buffer,"1", 1) ){
                        snprintf(buffer, MAXBUF, "%d\n active connections\n", nfds);
                    }
                    else if ( strncmp(buffer,"2", 1) ){
                        snprintf(buffer, MAXBUF, "Your server file desciptor is %d\n", events[n].data.fd);
                    }
                    else if ( strncmp(buffer,"3", 1) ){
                        time_t sysTime = time(NULL);
                        snprintf(buffer, MAXBUF, "The time is %s\n", ctime(&sysTime));
                    }
                    else if ( strncmp(buffer,"4", 1) ){
                        snprintf(buffer, MAXBUF, "Happy Birthday!!!!!\n");
                    }
                    else if ( strncmp(buffer,"5", 1) ){
                        snprintf(buffer, MAXBUF, "ZZZZZZZZZZZZZZZZZZZZZZZZZZZ\n");
                    }
                    else if ( strncmp(buffer,"passwd", 6) ){
                        snprintf(buffer, MAXBUF, "Function not found\n");
                    }
                    else if ( strncmp(buffer,"exit", 4) ){
                        snprintf(buffer, MAXBUF, "exit");
                    }
                    else if ( strncmp(buffer,"menu", 4) ){
                        snprintf( buffer, MAXBUF, "%s", this->_menu);
                    }
                    else {
                        snprintf( buffer, MAXBUF, "invalid command\n%s", this->_menu);
                    }*/

                    // send reply to client
                    if (send(events[n].data.fd, buffer, nbytes, 0) == -1) {
                        perror("send");
                    }
                }
            }
        }
    }
    close(epollfd);
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {
    close(this->_listSockFD);

}
