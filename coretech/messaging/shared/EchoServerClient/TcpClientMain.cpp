#include <iostream>
#include <unistd.h>
#include "TcpClient.h"

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 5556


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
  client.Connect(DEFAULT_HOST, DEFAULT_PORT);

  char sendBuf[1024];
  char recvBuf[1024];

  while( !(sendBuf[0] == 'q' && sendBuf[1] == 'u' && sendBuf[2] == 'i' && sendBuf[3] == 't') ) {
    std::cout << "Type message to send to server (type 'quit' to exit):\n";
    std::cin.getline(sendBuf, sizeof(sendBuf));
    
    int bytes_sent = client.Send(sendBuf, strlen(sendBuf)+1);  // +1 to include terminating null
    if (bytes_sent < 0) {
      break;
    }

    usleep(10000);

    int bytes_received = client.Recv(recvBuf, sizeof(recvBuf));
    if (bytes_received < 0) {
      cout << "Recv failed\n";
      break;
    } else if (bytes_received > 0) {
      cout << "Received: " << recvBuf << "\n";
    }
    
  }

  return(0);
}

