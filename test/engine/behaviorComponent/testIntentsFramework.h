/**
* File: testIntentsFramework.h
*
* Author: ross
* Created: Mar 20 2018
*
* Description: Framework to help with testing user/cloud/app intents that we consider "done"
*
* Copyright: Anki, Inc. 2018
*
**/

#include "json/json.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

namespace Anki {
namespace Vector {

enum class BehaviorID : uint16_t;
class IBehavior;
class TestBehaviorFramework;
class UserIntent;
  

class TestIntentsFramework
{
public:
  
  // where we're expecting to be listening for the intent
  enum class EHandledLocation : uint8_t {
    Anywhere=0, // any of the following
    HighLevelAI,
    Elsewhere, // todo: this probably needs to be broken up, eh?
  };
  
  TestIntentsFramework();
  
  // Check if, from the initial stack, the user intent results in the expected
  // behavior taking control of the stack. True if it takes control, false otherwise.
  // Pass in:
  //  1) Initial Behavior Stack
  //  2) User intent to send
  //  3) Behavior expected to handle the intent
  bool TestUserIntentTransition( TestBehaviorFramework& tbf,
                                 const std::vector<IBehavior*>& initialStack,
                                 UserIntent intentToSend,
                                 BehaviorID expectedIntentHandlerID,
                                 bool onlyCheckInStack = false );

  // returns true if the sample cloud intent can be parsed into a user intent. If requireMatch, then
  // that user intent can NOT be unmatched_intent. Also sets a non-null intent ptr with the matching intent
  bool TryParseCloudIntent( TestBehaviorFramework& tbf,
                            const std::string& cloudIntent,
                            UserIntent* intent=nullptr,
                            bool requireMatch=false );
  
  // Get a set of labels we consider "complete." This is a set instead of something unordered bc it's
  // useful to compare sorted sets. Provide an optional location to get labels based on where the
  // user intent is expected to be handled
  std::set<std::string> GetCompletedLabels( EHandledLocation location=EHandledLocation::Anywhere ) const;
  
  // Get the label that was associated with an intent.
  const std::string& GetLabelForIntent( const UserIntent& intent ) const;
  
  // obtain a UserIntent by its label you created in this class's cpp file
  UserIntent GetCompletedIntent( const std::string& label ) const;

private:
  
  struct IntentInfo {
    Json::Value def;
    EHandledLocation location;
  };
  
  void LoadCompletedIntents();
  
  static std::unordered_map<std::string, IntentInfo> _completedUserIntents;
};

}
}
