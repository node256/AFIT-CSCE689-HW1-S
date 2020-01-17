#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include "TCPClient.h"
#include "strfuncts.h"
#include "exceptions.h"


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
void TCPClient::connectTo(const char *ip_addr, unsigned short port) {

    // create connection socket and verify
    this->_clientSockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_clientSockFD < 0) { 
       throw socket_error("Unable to create socket.");
    }

    // set up IP address and port
    struct sockaddr_in cliAddr;
    cliAddr.sin_family = AF_INET;
    cliAddr.sin_port = htons(port);
    inet_aton(ip_addr, &cliAddr.sin_addr);

    // connect IP/port to socket file descriptor and verify
    if (connect(this->_clientSockFD, (struct sockaddr *) &cliAddr, sizeof(cliAddr)) < 0) {
       close(this->_clientSockFD);
       throw socket_error("Unable to connect");
    }
}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
// http://man7.org/linux/man-pages/man7/epoll.7.html
// Beej's Guide to Network Programming by Brian hall: https://beej.us/guide/bgnet/html/#poll
void TCPClient::handleConnection() {

    #define MAX_EVENTS 10

    struct sockaddr_storage clientAddr;
    socklen_t addrLen;

    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, epollfd;

    int nBytes;
    char inBuff[stdin_bufsize];
    char sockBuff[256];

    // Set up epoll file desciptor
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        throw socket_error("epoll create failed");
    }

    // Set up epoll control, add stdin and socked FD
    
    //  add STDIN to ep control
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        throw socket_error("STDIN add to epoll control failed");
    }

    // add cli socket to ep control
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = this->_clientSockFD;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->_clientSockFD, &ev) == -1) {
        throw socket_error("clientSock add to epoll control failed");
    }

    for (;;){
        // poll file descriptors for ready sockets
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            throw socket_error("poll wait fail");
        }

        // process events, transfer data
        for (int n = 0; n < nfds; ++n) {
            // capture and send client terminal input
            if (events[n].data.fd == STDIN_FILENO) {
                // clear buffer
                bzero(inBuff, stdin_bufsize);
                // capture stdin to buffer
                nBytes = read(STDIN_FILENO, inBuff, stdin_bufsize);
                // add check for \n

                if (send(_clientSockFD, inBuff, stdin_bufsize, 0) == -1) {
                    perror("send");
                }
            }
            // receive data from server
            if (events[n].data.fd == _clientSockFD ) {
                // clear buffer
                bzero(sockBuff, 256);
                nBytes = recv(_clientSockFD, sockBuff, 256, 0);

                // check for closed or errored connections
                if (nBytes < 0) {
                    throw socket_error("recieve error");
                }
                else {
                    // print data from server
                    std::cout << sockBuff;
                }
            }
        }
    }
    close(epollfd);
}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() {
    close(this->_clientSockFD);
}


