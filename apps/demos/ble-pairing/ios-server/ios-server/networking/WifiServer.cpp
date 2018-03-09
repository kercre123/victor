//
//  WifiServer.cpp
//  ios-server
//
//  Created by Paul Aluri on 2/2/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "WifiServer.h"

Anki::Switchboard::WifiServer::WifiServer(struct ev_loop* loop, int port) {
  _Loop = loop;
  _Port = port;
}

void Anki::Switchboard::WifiServer::StartServer() {
  struct ev_loop* defaultLoop = ev_default_loop(0);
  struct sockaddr_in6 addr;
  struct ev_io w_accept;
  
  int socket_fd = 0;
  uint32_t addr_len = sizeof(addr);
  
  if((socket_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
    perror("Could not create socket\n");
  }
  
  int no = 0;
  setsockopt(socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&no, sizeof(no));
  
  bzero(&addr, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(_Port);
  addr.sin6_addr = in6addr_any;
  
  bind(socket_fd, (sockaddr*)&addr, addr_len);
  listen(socket_fd, 0);
  
  ev_io_init(&w_accept, &AcceptCallback, socket_fd, EV_READ);
  ev_io_start(defaultLoop, &w_accept);
  
  ev_loop(defaultLoop, 0);
}

void Anki::Switchboard::WifiServer::AcceptCallback(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd;
  struct ev_io *w_client = (struct ev_io*) malloc(sizeof(struct ev_io));
  
  client_fd = accept(watcher->fd, (struct sockaddr*)&client_addr, &client_len);
  
  //_numberConnectedClients++;
  
  //ev_io_init(w_client, read_cb, client_fd, EV_READ);
  //ev_io_start(ev_default_loop(), w_client);
}
