/**
 * File: transferQueueMgr.h
 *
 * Author: Molly Jameson
 * Date:   1/29/2016
 *
 * Description: Manager for queueing data meant to be uploaded or downloaded at a later time
 *              Primary use cases for DAS and RAMS needing External delayed transfers since
 *              unlike overdrive Cozmo can't assume internet connection.
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "engine/util/transferQueue/transferQueueMgr.h"
#include "engine/util/http/createHttpAdapter.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include <string>

namespace Anki {

  namespace Util {

    TransferQueueMgr::TransferQueueMgr()
    : _queue(Dispatch::Create("TrnsQueueMgr"))
    , _numRequests(0)
    {
    }

    TransferQueueMgr::~TransferQueueMgr()
    {
      Dispatch::Stop(_queue);
      Dispatch::Release(_queue);
    }

    void TransferQueueMgr::ExecuteTransfers()
    {
      StartDataTransfer();
    }

    Signal::SmartHandle TransferQueueMgr::RegisterTask(const OnTransferReadyFunc& userFunc)
    {
      std::lock_guard<std::mutex> lock{_mutex};
      auto funcWrapper = [this, userFunc] (Dispatch::Queue* queue, const TaskCompleteFunc& completionFunc) {
        this->_numRequests++;
        userFunc(queue, completionFunc);
      };
      Signal::SmartHandle handle = _signal.ScopedSubscribe(funcWrapper);
      return handle;
    }

    void TransferQueueMgr::StartDataTransfer()
    {
      const bool noActiveRequests = _numRequests == 0;
      DEV_ASSERT(noActiveRequests, "TransferQueueMgr.StartDataTransfer.InvalidRequestCount");
      if (!noActiveRequests) {
        // perhaps previous request didn't finish yet, don't start more
        return;
      }
      std::unique_lock<std::mutex> lock{_mutex};
      _numRequests = 0;

      // set up completion func to notify this thread every time a task finishes
      auto completionFunc = [this] {
        Dispatch::Async(_queue, [this] {
          {
            // synchronously decrease the number of requests...
            std::lock_guard<std::mutex> lg{_mutex};
            _numRequests--;
          }
          // ...and notify the main thread that a task finished
          _waitVar.notify_one();
        });
      };
      // synchronously notify all tasks to begin on the queue
      Dispatch::Sync(_queue, [this, &completionFunc] {
        _signal.emit(_queue, completionFunc);
      });
      // ...and then wait for them all to finish (if no tasks are started this will just instantly continue)
      _waitVar.wait(lock, [this] { return _numRequests == 0; });
    }

  } // namespace Util
} // namespace Anki
