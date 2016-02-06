//
//  httpAdapter_android.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/19/15.
//
//

#ifndef __DriveEngine_HttpAdapter_android_H__
#define __DriveEngine_HttpAdapter_android_H__

#include "driveEngine/util/http/abstractHttpAdapter.h"
#include "driveEngine/valueObjects/jGlobalObjects/jGlobalObject.h"
#include <unordered_map>
#include <mutex>


namespace Anki {
  
namespace Util {
namespace Dispatch {
class Queue;
}
}
  
namespace DriveEngine {
  
typedef struct {
  HttpRequest request;
  Util::Dispatch::Queue* queue;
  HttpRequestCallback callback;
} HttpRequestResponseType;


class HttpAdapter : public IHttpAdapter
{
public:
  
  HttpAdapter(jobject jObject);
  ~HttpAdapter();
  
  void StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback);
  
  void ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody);
  
private:
  
  JGlobalObject _jObject;
  
  jmethodID _startRequestMethodID;
  
  uint64_t _hashCounter;
  
  std::unordered_map<uint64_t,HttpRequestResponseType> _requestResponseMap;
  
  std::mutex _callbackMutex;

};
  
}
}

#endif // defined(__DriveEngine_HttpAdapter_android_H__)
