#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Server.h"

class TCPServer : public Server 
{
public:
   TCPServer();
   ~TCPServer();

   void bindSvr(const char *ip_addr, unsigned short port);
   void listenSvr();
   void shutdown();

private:
   int _listSockFD;

};


#endif
