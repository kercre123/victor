/**
 * File: transferTaskHttp
 *
 * Author: baustin
 * Created: 7/20/16
 *
 * Description: TransferTask designed to execute a http request
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/util/transferQueue/transferTaskHttp.h"
#include "anki/cozmo/basestation/util/http/createHttpAdapter.h"

namespace Anki {
namespace Util {

TransferTaskHttp::TransferTaskHttp()
: _httpAdapter(CreateHttpAdapter())
{
}

void TransferTaskHttp::OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc)
{
  auto startRequestFunc = [this, queue, completionFunc] (const HttpRequest& request, const HttpRequestCallback& userCallback) {
    auto userCallbackWrapper = [userCallback, completionFunc] (const HttpRequest& innerRequest,
                                                               const int responseCode,
                                                               const std::map<std::string, std::string>& responseHeaders,
                                                               const std::vector<uint8_t>& responseBody) {
      // execute the user callback and tell the transfer manager we finished
      userCallback(innerRequest, responseCode, responseHeaders, responseBody);
      completionFunc();
    };
    _httpAdapter->StartRequest(request, queue, userCallbackWrapper);
  };
  OnReady(startRequestFunc);
}

}
}
