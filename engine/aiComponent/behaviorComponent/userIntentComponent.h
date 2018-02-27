/**
 * File: userIntentComponent.h
 *
 * Author: Brad Neuman
 * Created: 2018-02-01
 *
 * Description: Component to hold and query user intents (e.g. voice or app commands)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_UserIntentComponent_H__
#define __Engine_AiComponent_UserIntentComponent_H__

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include "json/json-forwards.h"

#include <mutex>

namespace Anki {
namespace Cozmo {

class BehaviorComponentCloudServer;
class CozmoContext;
class Robot;
class UserIntent;
class UserIntentMap;
  
namespace ExternalInterface{
struct AppIntent;
}


class UserIntentComponent : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable
{
public:
  
  UserIntentComponent(const Robot& robot, const Json::Value& userIntentMapConfig);

  ~UserIntentComponent();
  
  void Update();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Trigger word:
  //
  // The trigger word will be "pending" any time the audio process reports it. It will stay pending until it
  // is cleared, which should be very quickly in order to have a fast reaction. If a second trigger word comes
  // in (which should not happen and will likely cause a warning), they do _not_ queue, so it'll still just
  // count as pending and get cleared by a single clear
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  // Returns true if the trigger / wake word is pending
  bool IsTriggerWordPending() const;
  
  // Clear the pending trigger word. All subsequent calls to IsTriggerWordPending will return false until
  // another trigger word comes in.
  void ClearPendingTriggerWord();

  void SetTriggerWordPending();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // User Intent:
  //
  // A user intent is an enum member defined in userIntent.clad. It can come from a voice command but also
  // elsewhere (e.g. from the app once that is supported). Intents, like trigger words, should generally be
  // handled immediately, and they do not queue. If another intent comes in while the last one is still
  // pending, it will be overwritten
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Returns true if any user intent is pending
  bool IsAnyUserIntentPending() const;

  // Returns true if the specific user intent is pending
  bool IsUserIntentPending(UserIntentTag userIntent) const;

  // Same as above, but also get data
  bool IsUserIntentPending(UserIntentTag userIntent, UserIntent& extraData) const;

  // Clear the passed in user intent. If this is the current pending one, the pending intent will be cleared
  // until a new intent comes in. If not (e.g. another more-recent intent came in), this call will have no
  // effect (other than printing a warning)
  void ClearUserIntent(UserIntentTag userIntent);
  
  // The same as above, but also gets ownership of the intent, if userIntent matches what is pending
  UserIntent* ClearUserIntentWithOwnership(UserIntentTag userIntent);
  
  // clears but keeps the data in a list obtainable through TakePreservedUserIntentOwnership(). This
  // might be useful if a behavior expects that one of its delegates will consume the data. The
  // behavior clearingBehavior doing the clearing is now on the hook for someone else to consume it,
  // and clearingBehavior should ResetPreservedUserIntents() when it deactivates
  void ClearUserIntentWithPreservation(UserIntentTag userIntent, BehaviorID clearingBehavior);
  
  // if ClearUserIntentWithPreservation was used to clear the user intent instead of
  // ClearUserIntentWithOwnership, then the data can be obtained here. you now own it.
  UserIntent* TakePreservedUserIntentOwnership(UserIntentTag userIntent);
  
  // The clearingBehavior who called ClearUserIntentWithPreservation is on the hook for one of its
  // delegates to TakePreservedUserIntentOwnership. This will print an error if there are any
  // preserved intents, and then remove them. The clearingBehavior should call this OnDeactivated()
  void ResetPreservedUserIntents(BehaviorID clearingBehavior);

  // returns if an intent has recently gone unclaimed. or, reset the corresponding flag
  bool WasUserIntentUnclaimed() const { return _wasIntentUnclaimed; };
  void ResetUserIntentUnclaimed() { _wasIntentUnclaimed = false; };

  // replace the current pending user intent (if any) with this one. This will assert in dev if the
  // user intent data type is not void
  void SetUserIntentPending(UserIntentTag userIntent);
  
  // replace the current pending user intent (if any) with this one. 
  void SetUserIntentPending(UserIntent&& userIntent);

  
  // convert the passed in cloud intent to a user intent and set it pending. This will assert in dev
  // if the resulting user intent requires data, in which case you should call SetCloudIntentPendingFromJSON
  void SetCloudIntentPending(const std::string& cloudIntent);

  // convert the cloud JSON object into a user intent, mapping to the default intent if no cloud intent
  // matches. Returns true if successfully converted (including if it matched against the default), and false
  // if there was an error (e.g. malformed json, incorrect data fields, etc)
  bool SetCloudIntentPendingFromJSON(const std::string& cloudJsonStr);
  
  // get list of cloud/app intents from json
  std::vector<std::string> DevGetCloudIntentsList() const;
  std::vector<std::string> DevGetAppIntentsList() const;

private:
  
  // callback from the cloud
  void OnCloudData(std::string&& data);
  
  // message received from app
  void OnAppIntent( const ExternalInterface::AppIntent& appIntent );
  
  std::string GetServerName(const Robot& robot) const;
  
  void SendWebVizIntents();

  std::unique_ptr<UserIntentMap> _intentMap;

  bool _pendingTrigger = false;
  
  std::unique_ptr<UserIntent> _pendingIntent;
  
  struct Preserved {
    UserIntentTag tag;
    std::unique_ptr<UserIntent> intent;
    BehaviorID responsibleBehavior;
    float timeAdded;
  };
  std::vector<Preserved> _preservedIntents;

  // for debugging -- intents should be processed within one tick so track the ticks here
  size_t _pendingTriggerTick = 0;
  size_t _pendingIntentTick = 0;
  
  // holds cloud and trigger word event handles
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  std::mutex _mutex;
  std::string _pendingCloudIntent; // only pending for as long as it takes this thread to obtain a lock
  std::unique_ptr<BehaviorComponentCloudServer> _server;
  
  bool _wasIntentUnclaimed = false;
  
  const CozmoContext* _context = nullptr; // for webviz
  
  std::string _devLastReceivedCloudIntent;
  std::string _devLastReceivedAppIntent;
  
  

};

}
}


#endif
