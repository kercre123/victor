//
//  httpAdapter_vicos.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/19/15.
//
//

#ifndef __Anki_Util_HttpAdapterVicos_H__
#define __Anki_Util_HttpAdapterVicos_H__

#include "util/http/abstractHttpAdapter.h"


namespace Anki {
namespace Util {

namespace Dispatch {
class Queue;
}

class HttpAdapter : public IHttpAdapter
{
public:

  HttpAdapter(Util::Dispatch::Queue* queue = nullptr) {}
  virtual ~HttpAdapter() {}

  virtual void ShutdownAdapter() override {}

  virtual bool IsAdapterUp() override { return false; }

  virtual void StartRequest(HttpRequest request,
                    Util::Dispatch::Queue* queue,
                    HttpRequestCallback callback,
                    HttpRequestDownloadProgressCallback progressCallback = nullptr) override {}

  virtual void ExecuteCallback(const uint64_t hash,
                       const int responseCode,
                       std::map<std::string,std::string>& responseHeaders,
                       std::vector<uint8_t>& responseBody) override {}

  virtual void ExecuteDownloadProgressCallback(const uint64_t hash,
                                       const int64_t bytesWritten,
                                       const int64_t totalBytesWritten,
                                       const int64_t totalBytesExpectedToWrite) override {}
};

}
}

#endif // defined(__Anki_Util_HttpAdapterVicos_H__)
