/**
 * File: cannedAnimationContainer.h
 *
 * Authors: Andrew Stein
 * Created: 2014-10-22
 *
 * Description: Container for hard-coded or json-defined "canned" animations
 *              stored on the basestation and send-able to the physical robot.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
#define ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class IRobotMessageHandler;
  
  class CannedAnimationContainer
  {
  public:
    
    class KeyFrameList
    {
    public:
      ~KeyFrameList();
      
      void AddKeyFrame(RobotMessage* msg);
      
      bool IsEmpty() const {
        return _keyFrameMessages.empty();
      }
      
      const std::vector<RobotMessage*>& GetMessages() const {
        return _keyFrameMessages;
      }
      
    private:
      
      // TODO: Store some kind of KeyFrame wrapper that holds Message* instead of Message* directly
      std::vector<RobotMessage*> _keyFrameMessages;
    }; // class KeyFrameList
    
    
    CannedAnimationContainer();
    
    Result DefineHardCoded(); // called at construction
    
    Result DefineFromJson(Json::Value& jsonRoot, std::vector<s32>& loadedAnims);
    
    Result AddAnimation(const std::string& name);
    
    KeyFrameList* GetKeyFrameList(const std::string& name);
    
    s32 GetID(const std::string& name) const;
    
    // TODO: Add a way to ask how long an animation is
    //u16 GetLengthInMilliSeconds(const std::string& name) const;
    
    // Is there a better way to do this?
    Result Send(RobotID_t robotID, IRobotMessageHandler* msgHandler);
    
    void Clear();
    
  private:
    
    std::map<std::string, std::pair<s32, KeyFrameList> > _animations;
    
  }; // class Animation

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_CANNED_ANIMATION_CONTAINER_H
