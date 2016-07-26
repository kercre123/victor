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

#ifndef ANKI_COZMO_BASETATION_TRANSFERTASKHTTP_H
#define ANKI_COZMO_BASETATION_TRANSFERTASKHTTP_H

#include "anki/cozmo/basestation/util/transferQueue/transferTask.h"
#include "anki/cozmo/basestation/util/http/abstractHttpAdapter.h"
#include <functional>
#include <memory>

namespace Anki {
namespace Util {

class HttpAdapter;

class TransferTaskHttp : public TransferTask
{
public:
  TransferTaskHttp();
  virtual ~TransferTaskHttp() = default;

protected:
  using StartRequestFunc = std::function<void(const HttpRequest&, const HttpRequestCallback&)>;
  virtual void OnReady(const StartRequestFunc& requestFunc) = 0;

private:
  virtual void OnTransferReady(Dispatch::Queue* queue, const TransferQueueMgr::TaskCompleteFunc& completionFunc) override;

  std::unique_ptr<IHttpAdapter> _httpAdapter;
};

}
}

#endif
