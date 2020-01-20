#include <arpa/inet.h>
#include <vector>
#include "TCPConn.h"
#include "exceptions.h"


/**********************************************************************************************
 * TCPConn (constructor) 
 *
 **********************************************************************************************/

TCPConn::TCPConn() {

}

/**********************************************************************************************
 * TCPConn (destructor) 
 *
 **********************************************************************************************/

TCPConn::~TCPConn() {
    
}


/**********************************************************************************************
 * sendString - sends std::string over socket file descriptor
 *
 **********************************************************************************************/
// https://stackoverflow.com/questions/18670807/sending-and-receiving-stdstring-over-socket
int TCPConn::sendString(std::string msg, int sSockFD){
    // network byte order for data length
    unsigned int dataLength = htonl(msg.size());

    // send message size
    if (send(sSockFD, &dataLength, sizeof(unsigned int), MSG_CONFIRM) < 0){
        throw socket_error("msg length send");
    }
    // send message
    if (send(sSockFD, msg.c_str(), msg.size(), MSG_CONFIRM) < 0){
        throw socket_error("msg data send");
    }
    return 0;
}

/**********************************************************************************************
 * sendString - sends std::string over socket file descriptor
 *
 **********************************************************************************************/
// https://stackoverflow.com/questions/18670807/sending-and-receiving-stdstring-over-socket
int TCPConn::recvString(std::string &msg, int rSockFD){
    uint32_t dataLength;
    
    // recieve message size
    if (recv(rSockFD, &dataLength, sizeof(int), MSG_CONFIRM) < 0){
        throw socket_error("msg length send");
    }

    // switch to host byte order
    dataLength = ntohl(dataLength);

    // create recieve buffer and size to expected data
    std::vector<char> rcvBuf;
    rcvBuf.resize(dataLength,0x00);

    // receive message
    if (recv(rSockFD, &(rcvBuf[0]), dataLength, MSG_CONFIRM) < 0){
        throw socket_error("msg data send");
    }

    msg.assign(&(rcvBuf[0]),rcvBuf.size());

    return 0;    
}