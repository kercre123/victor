//
//  abstractHttpAdapter.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/3/15.
//  Updated by Molly Jameson on 2/4/16
//  Copyright (c) 2015 Anki. All rights reserved.
//
//

#ifndef __Util_Http_IHttpAdapter_H__
#define __Util_Http_IHttpAdapter_H__

#include "util/http/httpRequest.h"

#include <functional>
#include <string>
#include <map>
#include <vector>

namespace Anki {
namespace Util {

namespace Dispatch {
class Queue;
}

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
  virtual ~IHttpAdapter() {}

  enum class AdapterState {DOWN, UP};

  virtual void ShutdownAdapter() = 0;

  virtual bool IsAdapterUp() = 0;

  virtual void StartRequest(HttpRequest request,
                            Dispatch::Queue* queue,
                            HttpRequestCallback callback,
                            HttpRequestDownloadProgressCallback progressCallback = nullptr) = 0;

  virtual void ExecuteCallback(const uint64_t hash,
                               const int responseCode,
                               std::map<std::string,std::string>& responseHeaders,
                               std::vector<uint8_t>& responseBody) = 0;

  virtual void ExecuteDownloadProgressCallback(const uint64_t hash,
                                               const int64_t bytesWritten,
                                               const int64_t totalBytesWritten,
                                               const int64_t totalBytesExpectedToWrite) = 0;

  
};

} // namespace Util
} // namespace Anki

#endif
