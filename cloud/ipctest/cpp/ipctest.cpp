#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

static std::mutex _mutex;
static bool _done = false;
static const short _PORT = 19412;

static void receiveThread(std::function<int(char*,int)> receiveFunc) {
  char buffer[1024];
  while (!_done) {
    std::lock_guard<std::mutex> lock{_mutex};
    int received = receiveFunc(buffer, sizeof(buffer));
    if (received > 0) {
      const int endIdx = received >= sizeof(buffer) ? sizeof(buffer)-1 : received;
      buffer[endIdx] = '\0';
      std::cout << "Message received: " << buffer << std::endl;
    }
  }
}

static void server() {
  std::cout << "Starting server mode" << std::endl;
  UdpServer server;
  server.StartListening(_PORT);

  std::thread thread{receiveThread, [&server] (char* buf, int size) {
    return server.Recv(buf, size);
  }};

  std::string input;
  while (input != "done") {
    std::cin >> input;
    {
      std::lock_guard<std::mutex> lock{_mutex};
      server.Send(input.c_str(), (int)input.size()+1);
    }
  }
  _done = true;
  thread.join();
}

static void client() {
  std::cout << "Starting client mode (should be done after server is started)" << std::endl;
  UdpClient client;
  client.Connect("0.0.0.0", _PORT);

  std::thread thread{receiveThread, [&client] (char* buf, int size) {
    return client.Recv(buf, size);
  }};

  std::string input;
  while (input != "done") {
    std::cin >> input;
    {
      std::lock_guard<std::mutex> lock{_mutex};
      client.Send(input.c_str(), (int)input.size()+1);
    }
  }
  _done = true;
  thread.join();
}

int main(int argc, char **argv) {
  std::cout << "IPC test: C++ edition" << std::endl;
  if (argc < 2 || ((std::string{argv[1]}) == "server")) {
    server();
  }
  else {
    client();
  }
}
