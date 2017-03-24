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
#include "util/jni/jniUtils.h"
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

  HttpAdapter(Util::Dispatch::Queue* queue = nullptr);
  virtual ~HttpAdapter();

  virtual void ShutdownAdapter() override;

  virtual bool IsAdapterUp() override;

  virtual void StartRequest(HttpRequest request,
                    Util::Dispatch::Queue* queue,
                    HttpRequestCallback callback,
                    HttpRequestDownloadProgressCallback progressCallback = nullptr) override;

  virtual void ExecuteCallback(const uint64_t hash,
                       const int responseCode,
                       std::map<std::string,std::string>& responseHeaders,
                       std::vector<uint8_t>& responseBody) override;

  virtual void ExecuteDownloadProgressCallback(const uint64_t hash,
                                       const int64_t bytesWritten,
                                       const int64_t totalBytesWritten,
                                       const int64_t totalBytesExpectedToWrite) override;

private:
  struct HttpRequestResponseType {
    HttpRequest request;
    Util::Dispatch::Queue* queue;
    HttpRequestCallback callback;
    HttpRequestDownloadProgressCallback progressCallback;
  };

  void StartRequestInternal(HttpRequest&& request,
                            Util::Dispatch::Queue* queue,
                            HttpRequestCallback callback,
                            HttpRequestDownloadProgressCallback progressCallback);

  AdapterState GetAdapterState();
  void SetAdapterState(const AdapterState state);

  void GetHttpRequestResponseTypeFromHash(const uint64_t hash,
                                          HttpRequestResponseType& response);

  JGlobalObject _javaObject;

  jmethodID _startRequestMethodID;

  static std::atomic<uint64_t> _sHashCounter;

  std::unordered_map<uint64_t,HttpRequestResponseType> _requestResponseMap;

  std::mutex _callbackMutex;

  Util::Dispatch::Queue* _internalQueue;

  std::mutex _adapterStateMutex;
  AdapterState _adapterState;
};

}
}

#endif // defined(__Anki_Util_Jni_HttpAdapterAndroid_H__)
