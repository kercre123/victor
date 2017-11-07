/**
 * File: behaviorComponentCloudReceiver.h
 *
 * Author: Kevin M. Karol
 * Created: 10/30/17
 *
 * Description: Handles messages from the cloud that relate to the behavior component
 * And stores the derived information for the behavior component to access and use
 *
 * Copyright: Anki, Inc. 2017
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorComponentCloudReceiver_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorComponentCloudReceiver_H__

#include "clad/types/behaviorComponent/cloudIntents.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudServer.h"

#include "util/signals/simpleSignal_fwd.h"

#include <mutex>
#include <string>
#include <vector>

namespace Anki {
namespace Cozmo {

class Robot;

class BehaviorComponentCloudReceiver {
public:
  BehaviorComponentCloudReceiver(Robot& robot);

  // Returns true if there is an intent pending that matches
  // the intent type passed in
  bool IsIntentPending(CloudIntent intent);

  // Clears one intent of the specified type if it's pending
  void ClearIntentIfPending(CloudIntent intent);
  void AddPendingIntent(std::string&& intent);
  
private:

  std::mutex _mutex;
  std::vector<::Signal::SmartHandle> _eventHandles;
  std::vector<std::string> _pendingIntents;  
  BehaviorComponentCloudServer _server;
};

} // namespace Cozmo
} // namespace Anki

#endif //
