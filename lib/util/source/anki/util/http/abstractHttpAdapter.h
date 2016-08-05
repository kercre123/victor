//
//  abstractHttpAdapter.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/3/15.
//  Updated by Molly Jameson on 2/4/16
//  Copyright (c) 2015 Anki. All rights reserved.
//
//

#ifndef __Cozmo_Basestation_Util_Http_IHttpAdapter_H__
#define __Cozmo_Basestation_Util_Http_IHttpAdapter_H__

#include <functional>
#include <string>
#include <map>
#include <vector>
#include "util/http/httpRequest.h"

namespace Anki {
  
  namespace Util {
    namespace Dispatch {
      class Queue;
    }
  }
  
  namespace Util {
    
    using HttpRequestCallback =
      std::function<void (const HttpRequest& request,
                          const int responseCode,
                          const std::map<std::string, std::string>& responseHeaders,
                          const std::vector<uint8_t>& responseBody)>;
    using HttpRequestDownloadProgressCallback =
      std::function<void (const HttpRequest& request,
                          const int64_t bytesWritten,
                          const int64_t totalBytesWritten,
                          const int64_t totalBytesExpectedToWrite)>;
    static constexpr const int kHttpRequestCallbackTookTooLongMilliseconds = 250;
  // Abstract base class
    class IHttpAdapter
    {
    public:
      
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Methods
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      virtual ~IHttpAdapter() {}
      virtual void StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback) = 0;

      virtual void ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody) = 0;
    };
    
  }
}

#endif
