//
//  httpAdapter_android.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/19/15.
//
//

#ifndef __Anki_Util_Jni_HttpAdapterAndroid_H__
#define __Anki_Util_Jni_HttpAdapterAndroid_H__

#include "util/http/abstractHttpAdapter.h"
#include "util/http/httpRequest.h"
#include "util/jni/includeJni.h"
#include <atomic>
#include <unordered_map>
#include <mutex>


namespace Anki {
namespace Util {

namespace Dispatch {
class Queue;
}

class HttpAdapter : public IHttpAdapter
{
public:

  HttpAdapter();
  ~HttpAdapter();

  void StartRequest(const HttpRequest& request,
                    Util::Dispatch::Queue* queue,
                    HttpRequestCallback callback)
  {
    return StartRequest(request, queue, callback, nullptr);
  }

  void StartRequest(const HttpRequest& request,
                    Util::Dispatch::Queue* queue,
                    HttpRequestCallback callback,
                    HttpRequestDownloadProgressCallback progressCallback);

  void ExecuteCallback(const uint64_t hash,
                       const int responseCode,
                       const std::map<std::string,std::string>& responseHeaders,
                       const std::vector<uint8_t>& responseBody);

  void ExecuteDownloadProgressCallback(const uint64_t hash,
                                       const int64_t bytesWritten,
                                       const int64_t totalBytesWritten,
                                       const int64_t totalBytesExpectedToWrite);

private:
  struct HttpRequestResponseType {
    HttpRequest request;
    Util::Dispatch::Queue* queue;
    HttpRequestCallback callback;
    HttpRequestDownloadProgressCallback progressCallback;
  };

  void GetHttpRequestResponseTypeFromHash(const uint64_t hash,
                                          HttpRequestResponseType& response);

  jobject _javaObject;

  jmethodID _startRequestMethodID;

  static std::atomic<uint64_t> _sHashCounter;

  std::unordered_map<uint64_t,HttpRequestResponseType> _requestResponseMap;

  std::mutex _callbackMutex;

};

}
}

#endif // defined(__Anki_Util_Jni_HttpAdapterAndroid_H__)
