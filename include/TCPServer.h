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
   const char * _menu = "Enter one of the following commands\nhello\n1 #of active server connections IP\n2 Your file descriptor\n3 What time is it?\n4 Suprise\n5 How long did this take to program?\npasswd\nexit\nmenu";

};


#endif
