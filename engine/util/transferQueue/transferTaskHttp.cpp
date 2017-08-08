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

#include "engine/util/transferQueue/transferTaskHttp.h"
#include "engine/util/http/createHttpAdapter.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Util {

TransferTaskHttp::TransferTaskHttp()
: _httpAdapter(CreateHttpAdapter())
, _numTransfers(0)
{
}

void TransferTaskHttp::OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc)
{
  DEV_ASSERT(_numTransfers == 0, "TransferTaskHttp.InvalidStartTransferCount");
  _numTransfers = 0;
  bool transfersStarted = false;

  auto startRequestFunc = [this, queue, completionFunc, &transfersStarted] (const HttpRequest& request, const HttpRequestCallback& userCallback) {
    transfersStarted = true;
    auto userCallbackWrapper = [this, userCallback, completionFunc] (const HttpRequest& innerRequest,
                                                                     const int responseCode,
                                                                     const std::map<std::string, std::string>& responseHeaders,
                                                                     const std::vector<uint8_t>& responseBody) {
      // execute the user callback and tell the transfer manager we finished
      if (userCallback) {
        userCallback(innerRequest, responseCode, responseHeaders, responseBody);
      }
      _numTransfers--;
      if (_numTransfers == 0) {
        completionFunc();
      }
    };

    _numTransfers++;
    _httpAdapter->StartRequest(request, queue, std::move(userCallbackWrapper));
  };
  OnReady(startRequestFunc);

  // if OnReady resulted in transfers being started, this variable will have been modified
  if (!transfersStarted) {
    completionFunc();
  }
}

}
}
