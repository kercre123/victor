//
//  abstractHttpAdapter.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/3/15.
//  Copyright (c) 2015 Anki. All rights reserved.
//
//

#ifndef driveEngine_abstractHttpAdapter_h
#define driveEngine_abstractHttpAdapter_h

#include <functional>
#include <string>
#include <map>
#include <vector>
#include "httpRequest.h"

namespace Anki {
  
  namespace Util {
    namespace Dispatch {
      class Queue;
    }
  }
  
  namespace Util {
    
    using HttpRequestCallback = std::function<void (const HttpRequest& request, const int responseCode, const std::map<std::string, std::string>& responseHeaders, const std::vector<uint8_t>& responseBody)>;
    static constexpr const int kHttpRequestCallbackTookTooLongMilliseconds = 250;
  // Abstract base class
    class IHttpAdapter
    {
    public:
      
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Methods
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      
      virtual void StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback) = 0;

      virtual void ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody) = 0;
    };
    
  }
}

#endif
