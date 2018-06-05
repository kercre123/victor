/**
 * File: statusLogHandler.h
 *
 * Author: ross
 * Created: 2018-05-07
 *
 * Description: A handler for status messages and a store for recent statuses
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_StatusLogHandler_H__
#define __Engine_AiComponent_BehaviorComponent_StatusLogHandler_H__


#include "util/container/circularBuffer.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class CozmoContext;
namespace ExternalInterface {
  class Status;
}

class StatusLogHandler : private Util::noncopyable
{
public:
  explicit StatusLogHandler( const CozmoContext* context );
  
  void SetFeature( const std::string& featureName, const std::string& source );
  
private:
  
  void SaveStatusHistory( const ExternalInterface::Status& status );

  void SendStatusHistory();
  
  const CozmoContext* _context = nullptr;
  
  using StatusHistory = Util::CircularBuffer<std::pair<uint32_t,ExternalInterface::Status>>;
  std::unique_ptr<StatusHistory> _statusHistory;

  std::vector<Signal::SmartHandle> _signalHandles;
};

} // namespace Cozmo
} // namespace Anki

#endif
