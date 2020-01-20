#ifndef TCPCONN_H
#define TCPCONN_H

#include <string>
//#include "FileDesc.h"

class TCPConn 
{
public:
   TCPConn();
   ~TCPConn();

   //bool accept(SocketFD &server);

   virtual int sendString(std::string msg, int sSockFD);
   virtual int recvString(std::string &msg, int rSockFD);

 
private:

};


#endif
