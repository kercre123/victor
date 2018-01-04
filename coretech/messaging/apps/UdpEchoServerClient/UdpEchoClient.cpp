#include <iostream>
#include <unistd.h>
#include "UdpClient.h"

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 5557


using namespace std;

int main (int argc, char *argv[])
{
  // Client to echo server example
  UdpClient client;
  client.Connect(DEFAULT_HOST, DEFAULT_PORT);

  char sendBuf[1024];
  char recvBuf[1024];

  while( !(sendBuf[0] == 'q' && sendBuf[1] == 'u' && sendBuf[2] == 'i' && sendBuf[3] == 't') ) {
    std::cout << "Type message to send to server (type 'quit' to exit):\n";
    std::cin.getline(sendBuf, sizeof(sendBuf));

    if (strlen(sendBuf) > 0) {    
      int bytes_sent = client.Send(sendBuf, strlen(sendBuf)+1);  // +1 to include terminating null
      if (bytes_sent < 0) {
        break;
      }
    }

    usleep(10000);

    int bytes_received;
    do {
      bytes_received = client.Recv(recvBuf, sizeof(recvBuf));
      if (bytes_received < 0) {
        cout << "Recv failed\n";
        exit(-1);
      } else if (bytes_received > 0) {
        cout << "Received: " << recvBuf << "\n";
      }
    } while (bytes_received > 0);
    
  }

  return(0);
}

