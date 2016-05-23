/**
 * File: SosLoggerProvider
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
#ifndef __Util_Logging_SosLoggerProvider_H_
#define __Util_Logging_SosLoggerProvider_H_
#include "util/logging/iFormattedLoggerProvider.h"

namespace Anki {
  namespace Util {
    
    class SosLoggerProvider : public IFormattedLoggerProvider {
      
    public:
      SosLoggerProvider();
      
      SosLoggerProvider(const char* host, int portno);
      
      virtual ~SosLoggerProvider();
      
      void CloseSocket();
      void OpenSocket(const char* host = "localhost", int portno = 4444);

      void Log(ILoggerProvider::LogLevel logLevel, const std::string& message) override;
      void SetSoSTagEncoding(bool enable);
    private:
      
      int _sockfd;
      bool _sosTagEncode;
      bool _connected;
      
    };
  } // end namespace Util
} // end namespace Anki


#endif //__Util_Logging_SosLoggerProvider_H_
