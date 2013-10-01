#include <iostream>
#include "TcpClient.h"

using namespace std;

int main (int argc, char *argv[])
{
  /* 
  // Google request example 
  char buf[2048];
  const char* googleReq = "GET / HTTP/1.1\nhost: www.google.com\n\n";

  TcpClient client;
  client.Connect("www.google.com", "80");
  client.Send(googleReq, strlen(googleReq));
  
  client.Recv(buf);

  client.Disconnect();
  */

  // Client to echo server example
  TcpClient client;
  client.Connect("192.168.4.255", "5556");

  char sendBuf[1024];
  char recvBuf[1024];

  while( !(sendBuf[0] == 'q' && sendBuf[1] == 'u' && sendBuf[2] == 'i' && sendBuf[3] == 't') ) {
    std::cout << "Type message to send to server:\n";
    std::cin.getline(sendBuf, sizeof(sendBuf));
    
    client.Send(sendBuf, strlen(sendBuf)+1);  // +1 to include terminating null

    usleep(10000);

    client.Recv(recvBuf, sizeof(recvBuf));
  }

  return(0);
}

