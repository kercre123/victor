/**
 * File: cannedAnimationContainer.cpp
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

#include "cannedAnimationContainer.h"
#include "messageHandler.h"

#include "anki/cozmo/shared/cozmoTypes.h"

#include "anki/common/basestation/utils/logging/logging.h"

#include <cassert>


namespace Anki {
namespace Cozmo {
  
CannedAnimationContainer::CannedAnimationContainer()
{
  DefineHardCoded();
}
    
CannedAnimationContainer::KeyFrameList::~KeyFrameList()
{
  for(Message* msg : _keyFrameMessages) {
    if(msg != nullptr) {
      delete msg;
    }
  }
}

void CannedAnimationContainer::KeyFrameList::AddKeyFrame(Message* msg)
{
  if(msg != nullptr) {
    _keyFrameMessages.push_back(msg);
  } else {
    PRINT_NAMED_WARNING("Robot.KeyFrameList.AddKeyFrame.NullMessagePtr",
                        "Encountered NULL message adding canned keyframe.\n");
  }
}

Result CannedAnimationContainer::AddAnimation(const std::string& name) {
  auto retVal = _animations.find(name);
  if(retVal == _animations.end()) {
    const s32 animID = _animations.size();
    _animations[name].first = animID;
    
    PRINT_NAMED_INFO("CannedAnimationContainer.AddAnimation",
                     "Adding new animation named '%s' with ID=%d\n",
                     name.c_str(), animID);
    
    return RESULT_OK;
  } else {
    PRINT_NAMED_ERROR("CannedAnimationContainer.AddAnimation.DuplicateName",
                      "Animation named '%s' already exists. Not adding.\n",
                      name.c_str());
    return RESULT_FAIL;
  }
}

CannedAnimationContainer::KeyFrameList* CannedAnimationContainer::GetKeyFrameList(const std::string& name) {
  auto retVal = _animations.find(name);
  if(retVal == _animations.end()) {
    PRINT_NAMED_ERROR("CannedAnimationContainer.GetKeyFrameList.InvalidName",
                      "KeyFrameList requested for unknown animation '%s'.\n",
                      name.c_str());
    return nullptr;
  } else {
    return &retVal->second.second;
  }
}

s32 CannedAnimationContainer::GetID(const std::string& name) const {
  auto retVal = _animations.find(name);
  if(retVal == _animations.end()) {
    PRINT_NAMED_ERROR("CannedAnimationContainer.GetID.InvalidName",
                      "ID requested for unknown animation '%s'.\n",
                      name.c_str());
    return -1;
  } else {
    return retVal->second.first;
  }
}


void CannedAnimationContainer::Send(RobotID_t robotID, IMessageHandler* msgHandler)
{
  for(auto & cannedAnimationByName : _animations)
  {
    const std::string&  name         = cannedAnimationByName.first;
    const s32           animID       = cannedAnimationByName.second.first;
    const KeyFrameList& keyFrameList = cannedAnimationByName.second.second;
    
    if(keyFrameList.IsEmpty()) {
      PRINT_NAMED_WARNING("CannedAnimationContainer.Send.EmptyAnimation",
                          "Refusing to send empty '%s' animation with ID=%d.\n",
                          name.c_str(), animID);
    } else if (animID < 0) {
      PRINT_NAMED_WARNING("CannedAnimationContainer.Send.ZeroID",
                          "Refusing to send '%s' animation with invalid ID=-1.\n",
                          name.c_str());
    } else {
      
      // First send a clear command for this animation ID, before populating
      // it with keyframes
      MessageClearCannedAnimation clearMsg;
      clearMsg.animationID = static_cast<AnimationID_t>(animID);
      msgHandler->SendMessage(robotID, clearMsg);
      
      // Now send all the keyframe definition messages for this animation ID
      for(auto message : keyFrameList.GetMessages())
      {
        if(message != nullptr) {
          PRINT_NAMED_INFO("CannedAnimationContainer.Send",
                           "Sending '%s' animation defintion with ID=%d.\n",
                           name.c_str(), animID);
          
          msgHandler->SendMessage(robotID, *message);
        } else {
          PRINT_NAMED_WARNING("CannedAnimationContainer.Send.NullMessagePtr",
                              "Robot %d encountered NULL message sending canned animations.\n", robotID);
        }
      } // for each message
      
    } // if/else ladder for keyFrameList validity
    
  } // for each canned animation
  
} // SendCannedAnimations()


Result CannedAnimationContainer::DefineFromJson(Json::Value& jsonRoot)
{
  
  Json::Value::Members animationNames = jsonRoot.getMemberNames();
  
  for(auto const& animationName : animationNames)
  {
    // Must add the animation first to set its ID and allow the GetID and
    // GetKeyFrameList calls below to work.
    AddAnimation(animationName);
    
    const s32 animID = GetID(animationName);
    if(animID < 0) {
      return RESULT_FAIL;
    }
    
    const s32 numFrames = jsonRoot[animationName].size();
    for(s32 iFrame = 0; iFrame < numFrames; ++iFrame)
    {
      // Semi-hack to make sure each message has the animatino ID in it.
      // We do this here since the CreateFromJson() call below returns a
      // pointer to a base-class message (without accessible "animationID"
      // field).
      jsonRoot[animationName][iFrame]["animationID"] = animID;
      
      Message* kfMessage = Message::CreateFromJson(jsonRoot[animationName][iFrame]);
      if(kfMessage != nullptr) {
        KeyFrameList* keyFrames = GetKeyFrameList(animationName);
        if(keyFrames == nullptr) {
          return RESULT_FAIL;
        }
        keyFrames->AddKeyFrame(kfMessage);
      }
    } // for each frame
  } // for each animation
  
  return RESULT_OK;
}

Result CannedAnimationContainer::DefineHardCoded()
{
  
  /*
   //
   // FAST HEAD NOD - 3 fast nods
   //
   {
   // Start the nod
   MessageAddAnimKeyFrame_StartHeadNod* startNodMsg = new MessageAddAnimKeyFrame_StartHeadNod();
   startNodMsg->animationID = ANIM_HEAD_NOD;
   startNodMsg->relTime_ms = 0;
   startNodMsg->lowAngle  = DEG_TO_RAD(-10);
   startNodMsg->highAngle = DEG_TO_RAD( 10);
   startNodMsg->period_ms = 600;
   _cannedAnimations[ANIM_HEAD_NOD].AddKeyFrame(startNodMsg);
   
   
   // Stop the nod
   MessageAddAnimKeyFrame_StopHeadNod* stopNodMsg = new MessageAddAnimKeyFrame_StopHeadNod();
   stopNodMsg->animationID = ANIM_HEAD_NOD;
   stopNodMsg->relTime_ms = 1500;
   stopNodMsg->finalAngle = 0.f;
   _cannedAnimations[ANIM_HEAD_NOD].AddKeyFrame(stopNodMsg);
   
   } // FAST HEAD NOD
   */
  
  //
  // SLOW HEAD NOD - 2 slow nods
  //
  {
    AddAnimation("ANIM_HEAD_NOD_SLOW");
    
    const s32 animID = GetID("ANIM_HEAD_NOD_SLOW");
    assert(animID >= 0);
    
    KeyFrameList* keyFrames = GetKeyFrameList("ANIM_HEAD_NOD_SLOW");
    assert(keyFrames != nullptr);
    
    // Start the nod
    MessageAddAnimKeyFrame_StartHeadNod* startNodMsg = new MessageAddAnimKeyFrame_StartHeadNod();
    startNodMsg->animationID = animID;
    startNodMsg->relTime_ms = 0;
    startNodMsg->lowAngle  = DEG_TO_RAD(-25);
    startNodMsg->highAngle = DEG_TO_RAD( 25);
    startNodMsg->period_ms = 1200;
    keyFrames->AddKeyFrame(startNodMsg);
    
    
    // Stop the nod
    MessageAddAnimKeyFrame_StopHeadNod* stopNodMsg = new MessageAddAnimKeyFrame_StopHeadNod();
    stopNodMsg->animationID = animID;
    stopNodMsg->relTime_ms = 2400;
    stopNodMsg->finalAngle = 0.f;
    keyFrames->AddKeyFrame(stopNodMsg);
  } // SLOW HEAD NOD
  
  /*
   //
   // LIFT NOD
   //
   {
   // Start the nod
   MessageAddAnimKeyFrame_StartLiftNod* startNodMsg = new MessageAddAnimKeyFrame_StartLiftNod();
   startNodMsg->animationID = ANIM_LIFT_NOD;
   startNodMsg->relTime_ms = 0;
   startNodMsg->lowHeight_mm  = 60;
   startNodMsg->highHeight_mm = 75;
   startNodMsg->period_ms = 300;
   _cannedAnimations[ANIM_LIFT_NOD].AddKeyFrame(startNodMsg);
   
   // Stop the nod
   MessageAddAnimKeyFrame_StopLiftNod* stopNodMsg = new MessageAddAnimKeyFrame_StopLiftNod();
   stopNodMsg->animationID = ANIM_LIFT_NOD;
   stopNodMsg->relTime_ms = 1200;
   stopNodMsg->finalHeight_mm = 60;
   _cannedAnimations[ANIM_LIFT_NOD].AddKeyFrame(stopNodMsg);
   
   } // LIFT NO
   */
  
  /*
  //
  // BLINK
  //
  {
    AddAnimation("ANIM_BLINK");
    
    const s32 animID = GetID("ANIM_BLINK");
    assert(animID >= 0);
    
    KeyFrameList* keyFrames = GetKeyFrameList("ANIM_BLINK");
    assert(keyFrames != nullptr);
    
    
    MessageAddAnimKeyFrame_SetLEDColors setLEDmsgProto;
    setLEDmsgProto.animationID = animID;
    setLEDmsgProto.transitionIn  = KF_TRANSITION_INSTANT;
    setLEDmsgProto.transitionOut = KF_TRANSITION_INSTANT;
    
    // Start with all eye segments on:
    setLEDmsgProto.relTime_ms = 0;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_BOTTOM]  = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_LEFT]    = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_TOP]     = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_BOTTOM] = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_TOP]    = LED_BLUE;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_SetLEDColors(setLEDmsgProto));
    
    // Turn off top/bottom segments first
    setLEDmsgProto.relTime_ms = 1700;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_LEFT]    = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_TOP]     = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_TOP]    = LED_OFF;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_SetLEDColors(setLEDmsgProto));
    
    // Turn off all segments shortly thereafter
    setLEDmsgProto.relTime_ms = 1750;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_LEFT]    = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_RIGHT]   = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_TOP]     = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_LEFT]   = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_RIGHT]  = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_TOP]    = LED_OFF;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_SetLEDColors(setLEDmsgProto));
    
    // Turn on left/right segments first
    setLEDmsgProto.relTime_ms = 1850;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_LEFT]    = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_TOP]     = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_TOP]    = LED_OFF;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_SetLEDColors(setLEDmsgProto));
    
    // Turn on all segments shortly thereafter
    setLEDmsgProto.relTime_ms = 1900;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_BOTTOM]  = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_LEFT]    = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_LEFT_EYE_TOP]     = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_BOTTOM] = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
    setLEDmsgProto.LEDcolors[LED_RIGHT_EYE_TOP]    = LED_BLUE;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_SetLEDColors(setLEDmsgProto));
  } // BLINK
  */
  
  //
  // BACK_AND_FORTH_EXCITED
  //
  {
    AddAnimation("ANIM_BACK_AND_FORTH_EXCITED");
    
    const s32 animID = GetID("ANIM_BACK_AND_FORTH_EXCITED");
    assert(animID >= 0);
    
    KeyFrameList* keyFrames = GetKeyFrameList("ANIM_BACK_AND_FORTH_EXCITED");
    assert(keyFrames != nullptr);
    
    
    MessageAddAnimKeyFrame_DriveLine driveLineMsgProto;
    driveLineMsgProto.animationID = animID;
    driveLineMsgProto.transitionIn  = KF_TRANSITION_EASE_IN;
    driveLineMsgProto.transitionOut = KF_TRANSITION_EASE_OUT;
    
    driveLineMsgProto.relTime_ms = 300;
    driveLineMsgProto.relativeDistance_mm = -9;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_DriveLine(driveLineMsgProto));
    
    driveLineMsgProto.relTime_ms = 600;
    driveLineMsgProto.relativeDistance_mm = 9;
    keyFrames->AddKeyFrame(new MessageAddAnimKeyFrame_DriveLine(driveLineMsgProto));
    
  } // BACK_AND_FORTH_EXCITED
  
  return RESULT_OK;
  
} // DefineHardCodedAnimations()

} // namespace Cozmo
} // namespace Anki