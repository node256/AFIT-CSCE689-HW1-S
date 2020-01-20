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
#include "exceptions.h"
#include "TCPConn.h"

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
    
    // create listener socket and verify
    this->_listSockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_listSockFD < 0) { 
       throw socket_error("Unable to create socket.");
    }

    // set up IP address and port
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    inet_aton(ip_addr, &servAddr.sin_addr);

    // bind IP/port to socket file descriptor and verify
    if (bind(this->_listSockFD, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
       close(this->_listSockFD);
       throw socket_error("Unable to bind");
    }
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

    TCPConn tcpConn;

    int nBytes;
    char buffer[MAXBUF];


    bool exit;

    // start and verify listening on socket
    if (listen(this->_listSockFD, 10) < 0 ){
        throw socket_error("Unable to listen");
    }

    // Set up epoll file desciptor
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        throw socket_error("epoll create failed");
    }

    // Set up epoll control and add listening socket
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = this->_listSockFD;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->_listSockFD, &ev) == -1) {
        throw socket_error("listener add to epoll control failed");
    }

    // accepting connections and processing commands
    for (;;){
        // poll file descriptors for ready sockets
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            throw socket_error("poll wait fail");
        }

        // process events, accept connections, transfer data
        for (int n = 0; n < nfds; ++n) {
            // accept new connections
            if (events[n].data.fd == this->_listSockFD) {
                connSockFD = accept(this->_listSockFD, (struct sockaddr *) &clientAddr, &addrLen);
                if (connSockFD == -1) {
                    throw socket_error("Unable to accept");
                }

                // add new connection to poll list
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connSockFD;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connSockFD, &ev) == -1) {
                    throw socket_error("connect add to epoll control failed");
                }

                if ((tcpConn.sendString(this->_menu, connSockFD)) < 0){
                    throw socket_error("init menu send failed");
                }
                /*if (send(ev.data.fd, this->_menu, strlen(this->_menu), 0) < 0){
                    throw socket_error("init menu send failed");
                    
                }*/

            // process commands, send/recive data
            } else {
                // zeroize buffer
                bzero(buffer, MAXBUF);

                // read from client to buffer
                nBytes = recv(events[n].data.fd, buffer, sizeof(buffer), 0);

                // check for closed or errored connections
                if (nBytes <= 0) {
                    if (nBytes == 0) {
                        // Connection closed
                        printf("pollserver: socket %d hung up\n", events[n].data.fd);
                    }
                    else { 
                        throw socket_error("connection error");
                    }

                    // close and cleanup socket
                    close(events[n].data.fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                } 
                else {
                    // process client input and send response
                    if ( strncmp(buffer,"exit", 4) == 0 ){
                        snprintf(buffer, MAXBUF, "Closing connection\n");
                        exit = true;
                    }
                    else if ( strncmp(buffer,"hello", 5) == 0 ){
                        snprintf(buffer, MAXBUF, "Hello Dave\n");
                    }
                    else if ( strncmp(buffer,"1", 1) == 0 ){
                        snprintf(buffer, MAXBUF, "I don't know\n");
                    }
                    else if ( strncmp(buffer,"2", 1) == 0 ){
                        snprintf(buffer, MAXBUF, "Your server file desciptor is %d\n", events[n].data.fd);
                    }
                    else if ( strncmp(buffer,"3", 1) == 0 ){
                        time_t sysTime = time(NULL);
                        snprintf(buffer, MAXBUF, "The time is %s", ctime(&sysTime));
                    }
                    else if ( strncmp(buffer,"4", 1) == 0 ){
                        snprintf(buffer, MAXBUF, "Happy Birthday!!!!!\n");
                    }
                    else if ( strncmp(buffer,"5", 1) == 0){
                        snprintf(buffer, MAXBUF, "ZZZZZZZZZZZZZZZZZZZZZZZZZZZ\n");
                    }
                    else if ( strncmp(buffer,"passwd", 6) == 0 ){
                        snprintf(buffer, MAXBUF, "Function not found\n");
                    }
                    else if ( strncmp(buffer,"menu", 4) == 0 ){
                        snprintf( buffer, MAXBUF, "%s", this->_menu);
                    }
                    else {
                        snprintf( buffer, MAXBUF, "invalid command\n%s", this->_menu);
                    }

                    // send reply to client
                    if (send(events[n].data.fd, buffer, strlen(buffer), 0) == -1) {
                        throw socket_error("send response");
                    }
                    if (exit){
                        // close and cleanup socket at client request
                        printf("pollserver: socket %d hung up\n", events[n].data.fd);
                        close(events[n].data.fd);
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                        exit = false;
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
