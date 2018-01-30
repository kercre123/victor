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

#include "util/entityComponent/iManageableComponent.h"

#include "util/helpers/noncopyable.h"

#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class UserIntentMap;

class UserIntentComponent : public IManageableComponent, private Util::noncopyable
{
public:
  
  explicit UserIntentComponent(const Json::Value& userIntentMapConfig);

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
  // A user intent is a string defined in user_intent_map.json. It can come from a voice command but also
  // elsewhere (e.g. from the app once that is supported). Intents, like trigger words, should generally be
  // handled immediately, and they do not queue. If another intent comes in while the last one is still
  // pending, it will be overwritten
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Returns true if the passed in string is a valid user intent. This function should be used to verify all
  // data-defined intents
  bool IsValidUserIntent(const std::string& userIntent) const;

  // Returns true if any user intent is pending
  bool IsAnyUserIntentPending() const;

  // Returns true if the specific user intent is pending
  bool IsUserIntentPending(const std::string& userIntent) const;

  // Clear the passed in user intent. If this is the current pending one, the pending intent will be cleared
  // until a new intent comes in. If not (e.g. another more-recent intent came in), this call will have no
  // effect (other than printing a warning)
  void ClearUserIntent(const std::string& userIntent);

  // replace the current pending user intent (if any) with this one
  void SetUserIntentPending(const std::string& userIntent);

  // convert the passed in cloud intent to a user intent and set it pending
  void SetCloudIntentPending(const std::string& cloudIntent);

  // convert the cloud JSON object into a user intent, mapping to the default intent if no cloud intent
  // matches. Returns true if successfully converted (including if it matched against the default), and false
  // if there was an error (e.g. malformed json, incorrect data fields, etc)
  bool SetCloudIntentFromJSON(const std::string& cloudJsonStr);

private:

  std::unique_ptr<UserIntentMap> _intentMap;

  bool _pendingTrigger = false;
  std::string _pendingIntent;

  // for debugging -- intents should be processed within one tick so track the ticks here
  size_t _pendingTriggerTick = 0;
  size_t _pendingIntentTick = 0;

};

}
}


#endif
