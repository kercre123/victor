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

#include "engine/aiComponent/behaviorComponent/userIntentComponent_fwd.h"

#include "clad/cloud/mic.h"
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

// helper to avoid .h dependency on userIntent.clad
const UserIntentSource& GetIntentSource(const UserIntentData& intentData);

class UserIntentComponent : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable
{
public:
  
  UserIntentComponent(const Robot& robot, const Json::Value& userIntentMapConfig);

  ~UserIntentComponent();
  
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;

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
  // pending, it will be overwritten. There is up to one _pending_ intent and one _active_ intent at any time
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Returns true if any user intent is pending
  bool IsAnyUserIntentPending() const;

  // Returns true if the specific user intent is pending
  bool IsUserIntentPending(UserIntentTag userIntent) const;

  // Same as above, but also get data
  bool IsUserIntentPending(UserIntentTag userIntent, UserIntent& extraData) const;

  // Activate the pending user intent. Owner string is stored for debugging because you are responsible to
  // deactivate the intent if you call this function directly. If the given userIntent is pending, this will
  // move it from pending to active (it will no longer be pending) and return a pointer to the full intent
  // data. Otherwise, it will print warnings and return nullptr
  UserIntentPtr ActivateUserIntent(UserIntentTag userIntent, const std::string& owner);

  // Must be called when a user intent is no longer active (generally meaning that the behavior that was
  // handling the intent has stopped).
  void DeactivateUserIntent(UserIntentTag userIntent);
  
  // Check if an intent tag is currently active
  bool IsUserIntentActive(UserIntentTag userIntent) const;

  // If the given intent is active, return the associated data, otherwise return nullptr.
  UserIntentPtr GetUserIntentIfActive(UserIntentTag forIntent) const;

  // If any intent is active, return the associated data, otherwise return nullptr. In general, most cases
  // should use the above version that checks that the explicit user intent is matching what the caller
  // expects
  UserIntentPtr GetActiveUserIntent() const;

  // A helper function to drop a user intent without responding to it. This is meant to be called for a
  // pending user intent and will make it no longer pending without ever making it active. This is generally
  // not what you want, but can be useful to explicitly ignore a command
  void DropUserIntent(UserIntentTag userIntent);

  // returns if an intent has recently gone unclaimed. or, reset the corresponding flag
  bool WasUserIntentUnclaimed() const { return _wasIntentUnclaimed; };
  void ResetUserIntentUnclaimed() { _wasIntentUnclaimed = false; };

  // replace the current pending user intent (if any) with this one. This will assert in dev if the user
  // intent data type is not void. These should only be used for test and dev purposes (e.g. webviz), not
  // directly used to express an intent
  void DevSetUserIntentPending(UserIntentTag userIntent, const UserIntentSource& source);
  void DevSetUserIntentPending(UserIntent&& userIntent, const UserIntentSource& source);
  void DevSetUserIntentPending(UserIntentTag userIntent);
  void DevSetUserIntentPending(UserIntent&& userIntent);

  // this allows us to temporarilty disable the warning when we haven't responded to a pending intent
  // useful when we know a behavior down the line will consume the intent, but we still
  // have some work to do at the moment
  // note: this is re-enabled with each new intent
  void SetUserIntentTimeoutEnabled(bool isEnabled);
  
  // convert the passed in cloud intent to a user intent and set it pending. This will assert in dev
  // if the resulting user intent requires data, in which case you should call SetCloudIntentPendingFromJSON
  void SetCloudIntentPending(const std::string& cloudIntent);

  // convert the cloud JSON object into a user intent, mapping to the default intent if no cloud intent
  // matches. Returns true if successfully converted (including if it matched against the default), and false
  // if there was an error (e.g. malformed json, incorrect data fields, etc)
  bool SetCloudIntentPendingFromJSON(const std::string& cloudJsonStr);
  bool SetCloudIntentPendingFromJSONValue(Json::Value cloudJson);
  
  // get list of cloud/app intents from json
  std::vector<std::string> DevGetCloudIntentsList() const;
  std::vector<std::string> DevGetAppIntentsList() const;

private:
  
  // callback from the cloud
  void OnCloudData(CloudMic::Message&& data);
  
  // message received from app
  void OnAppIntent( const ExternalInterface::AppIntent& appIntent );
  
  std::string GetServerName(const Robot& robot) const;
  
  void SendWebVizIntents();

  void SetUserIntentPending(UserIntentTag userIntent, const UserIntentSource& source);
  void SetUserIntentPending(UserIntent&& userIntent, const UserIntentSource& source);

  static size_t sActivatedIntentID;

  std::unique_ptr<UserIntentMap> _intentMap;

  bool _pendingTrigger = false;
  
  std::unique_ptr<UserIntentData> _pendingIntent;
  std::shared_ptr<UserIntentData> _activeIntent;
  std::string _activeIntentOwner;
  
  // for debugging -- intents should be processed within one tick so track the ticks here
  size_t _pendingTriggerTick = 0;
  size_t _pendingIntentTick = 0;
  bool _pendingIntentTimeoutEnabled = true;
  
  // holds cloud and trigger word event handles
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  std::mutex _mutex;
  CloudMic::Message _pendingCloudIntent; // only pending for as long as it takes this thread to obtain a lock
  std::unique_ptr<BehaviorComponentCloudServer> _server;
  
  bool _wasIntentUnclaimed = false;
  
  const CozmoContext* _context = nullptr; // for webviz
  
  std::string _devLastReceivedCloudIntent;
  std::string _devLastReceivedAppIntent;
  
  

};

}
}


#endif
