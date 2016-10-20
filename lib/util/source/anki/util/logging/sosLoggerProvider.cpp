/**
 * File: sosLoggerProvider
 *
 * Author: Trevor Dasch
 * Created: 2/9/16
 *
 * Description: Opens a socket to an SOSMax
 * Instance and sends logs
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "util/logging/sosLoggerProvider.h"
#include "util/logging/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <sstream>

namespace Anki {
  namespace Util {
    
    //http://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string
    std::string xml_encode(const std::string& data) {
      std::string buffer;
      buffer.reserve((size_t)(data.size() * 1.1)); // add 10% extra space
      for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
          case '&':  buffer.append("&amp;");       break;
          case '\"': buffer.append("&quot;");      break;
          case '\'': buffer.append("&apos;");      break;
          case '<':  buffer.append("&lt;");        break;
          case '>':  buffer.append("&gt;");        break;
          default:   buffer.append(&data[pos], 1); break;
        }
      }
      return buffer;
    }
    
    SosLoggerProvider::SosLoggerProvider() : _sosTagEncode(true), _connected(false)
    {
      OpenSocket("localhost", 4444);
    }
    
    SosLoggerProvider::SosLoggerProvider(const char* host, int portno) : _connected(false)
    {
      OpenSocket(host, portno);
    }
    
    SosLoggerProvider::~SosLoggerProvider()
    {
      CloseSocket();
    }
    
    void SosLoggerProvider::Log(ILoggerProvider::LogLevel logLevel, const std::string& message)
    {
      // if we aren't connected, just dump the logs
      if(!_connected) {
        return;
      }
      auto encodedMessage = xml_encode(message);
      
      std::string sosString;
      if (_sosTagEncode) {
        sosString = "!SOS<showMessage key='" + std::string(GetLogLevelString(logLevel)) + "'>" + encodedMessage + "</showMessage>\n";
      } else {
        sosString = encodedMessage;
      }
      
      ssize_t n = write(_sockfd,sosString.c_str(),sosString.size()+1);
      
      if (n == -1) {
        fprintf(stderr,"SOS - error writing to socket. Closing connection.\n");
        CloseSocket();
        //TODO: Try reopening?
      }
    }
    
    void SosLoggerProvider::CloseSocket() {
      if(_connected) {
        close(_sockfd);
        _sockfd = -1;
        _connected = false;
      }
    }
    
    void SosLoggerProvider::OpenSocket(const char* host, int portno)
    {
      CloseSocket();
      
      struct sockaddr_in serv_addr;
      struct hostent *server;
      
      _sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (_sockfd < 0) {
        fprintf(stderr,"SOS - error opening socket\n");
        return;
      }
      server = gethostbyname(host);
      if (server == NULL) {
        fprintf(stderr,"SOS - no such host %s\n", host);
        return;
      }
      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
      serv_addr.sin_port = htons(portno);
      if (connect(_sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        PRINT_NAMED_INFO("SOS", "Failed to connect to SOS server");
      }
      else {
        PRINT_NAMED_INFO("SOS", "Connected to SOS server");
        _connected = true;
      }
    }
    
    void SosLoggerProvider::SetSoSTagEncoding(bool enable) {
      _sosTagEncode = enable;
    }
  } // end namespace Util
} // end namespace Anki
