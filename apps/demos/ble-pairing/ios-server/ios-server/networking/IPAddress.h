//
//  IPAddress.h
//  ios-server
//
//  Created by Paul Aluri on 2/2/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef IPAddress_h
#define IPAddress_h

#include <stdio.h>
#include <unistd.h>
#include <string.h> /* For strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

class IP {
public:
  static void printIpAddress() {
    int fd;
    struct ifreq ifr;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    
    /* I want an IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "en0", IFNAMSIZ-1);
    
    ioctl(fd, SIOCGIFADDR, &ifr);
    
    close(fd);
    
    /* Display result */
    printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    
    unsigned char* addr = (unsigned char*)&(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    printf("<");
    for(int i = 0; i < sizeof(uint32_t); i++) {
      printf("%u ", addr[i]);
    }
    printf(">\n");
  }
};

#endif /* IPAddress_h */
