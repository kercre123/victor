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
#include "soundManager.h"

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

  /*
  u16 CannedAnimationContainer::GetLengthInMilliSeconds(const std::string& name) const
  {
    auto result = _animations.find(name);
    if(result == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetLengthInMilliSeconds.InvalidName",
                        "Length requested for unknown animation '%s'.\n",
                        name.c_str());
      return 0;
    } else {
      u16 length = 0;
      
      const KeyFrameList& anim = result->second.second;
      
    }
  } // GetLengthInMilliSeconds()
   */
   
  Result CannedAnimationContainer::Send(RobotID_t robotID, IMessageHandler* msgHandler)
  {
    Result lastResult = RESULT_OK;
    
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
                             "Sending '%s' animation definition with ID=%d.\n",
                             name.c_str(), animID);
            
            lastResult = msgHandler->SendMessage(robotID, *message);
            if(lastResult != RESULT_OK) {
              PRINT_NAMED_ERROR("CannedAnimationContainer.Send.SendMessageFail",
                                "Failed to send a keyframe message.\n");
              return lastResult;
            }
          } else {
            PRINT_NAMED_WARNING("CannedAnimationContainer.Send.NullMessagePtr",
                                "Robot %d encountered NULL message sending canned animations.\n", robotID);
          }
        } // for each message
        
      } // if/else ladder for keyFrameList validity
      
    } // for each canned animation
    
    return lastResult;
    
  } // SendCannedAnimations()

  static KeyFrameTransitionType GetTransitionType(const Json::Value& json)
  {
    static const std::map<std::string, KeyFrameTransitionType> LUT = {
      {"EASE_IN",  KF_TRANSITION_EASE_IN},
      {"EASE_OUT", KF_TRANSITION_EASE_OUT},
      {"STEP",     KF_TRANSITION_INSTANT},
      {"INSTANT",  KF_TRANSITION_INSTANT},
      {"LINEAR",   KF_TRANSITION_LINEAR}
    };
    
    const KeyFrameTransitionType DEFAULT = KF_TRANSITION_LINEAR;
    
    if(json.isString()) {
      
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("GetTransitionType",
                            "Unknown transition type '%s', returning default.\n",
                            json.asString().c_str());
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isIntegral()) {
      return static_cast<KeyFrameTransitionType>(json.asInt());
    } else {
      PRINT_NAMED_WARNING("GetTransitionType",
                          "Not sure how to convert stored transition type, returning default.\n");
      return DEFAULT;
    }
  } // GetTransitionType()
  
  
  static EyeShape GetEyeShape(const Json::Value& json)
  {
    static const std::map<std::string, EyeShape> LUT = {
      {"CURRENT_SHAPE",   EYE_CURRENT_SHAPE},
      {"OPEN",            EYE_OPEN},
      {"HALF",            EYE_HALF},
      {"SLIT",            EYE_SLIT},
      {"CLOSED",          EYE_CLOSED},
      {"OFF_PUPIL_UP",    EYE_OFF_PUPIL_UP},
      {"OFF_PUPIL_DOWN",  EYE_OFF_PUPIL_DOWN},
      {"OFF_PUPIL_LEFT",  EYE_OFF_PUPIL_LEFT},
      {"OFF_PUPIL_RIGHT", EYE_OFF_PUPIL_RIGHT},
      {"ON_PUPIL_UP",     EYE_ON_PUPIL_UP},
      {"ON_PUPIL_DOWN",   EYE_ON_PUPIL_DOWN},
      {"ON_PUPIL_LEFT",   EYE_ON_PUPIL_LEFT},
      {"ON_PUPIL_RIGHT",  EYE_ON_PUPIL_RIGHT}
    };
    
    const EyeShape DEFAULT = EYE_CURRENT_SHAPE;
    
    if(json.isString()) {
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("GetEyeShape", "Unknown eye shape '%s', returning default.\n",
                            json.asString().c_str());
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isIntegral()) {
      return static_cast<EyeShape>(json.asInt());
    } else {
      PRINT_NAMED_WARNING("GetEyeShape",
                          "Not sure how to convert stored eye shape, returning default.\n");
      return DEFAULT;
    }
  } // GetEyeShape()
  
  
  static WhichEye GetWhichEye(const Json::Value& json)
  {
    static const std::map<std::string, WhichEye> LUT = {
      {"BOTH",   EYE_BOTH},
      {"LEFT",   EYE_LEFT},
      {"RIGHT",  EYE_RIGHT}
    };
    
    const WhichEye DEFAULT = EYE_BOTH;
    
    if(json.isString()) {
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_NAMED_WARNING("GetWhichEye", "Unknown eye '%s', returning default.\n",
                            json.asString().c_str());
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isIntegral()) {
      return static_cast<WhichEye>(json.asInt());
    } else {
      PRINT_NAMED_WARNING("GetWhichEye",
                          "Not sure how to convert stored eye, returning default.\n");
      return DEFAULT;
    }
  } // GetEyeShape()
  

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
        Json::Value& jsonFrame = jsonRoot[animationName][iFrame];
        
        // Semi-hack to make sure each message has the animatino ID in it.
        // We do this here since the CreateFromJson() call below returns a
        // pointer to a base-class message (without accessible "animationID"
        // field).
        jsonFrame["animationID"] = animID;
        
        // Convert transition type strings to values:
        if(!jsonFrame.isMember("transitionIn")) {
          PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.NoTransitionIn",
                            "Missing 'transitionIn' field for '%s' frame of '%s' animation.\n",
                            jsonFrame["Name"].asString().c_str(),
                            animationName.c_str());
          return RESULT_FAIL;
        }
        jsonFrame["transitionIn"] = GetTransitionType(jsonFrame["transitionIn"]);
        
        if(!jsonFrame.isMember("transitionOut")) {
          PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.NoTransitionOut",
                            "Missing 'transitionOut' field for '%s' frame of '%s' animation.\n",
                            jsonFrame["Name"].asString().c_str(),
                            animationName.c_str());
          return RESULT_FAIL;
        }
        jsonFrame["transitionOut"] = GetTransitionType(jsonFrame["transitionOut"]);
        
        // Convert sound name (if specified) to SoundID (robot doesn't use strings):
        if(jsonFrame.isMember("soundID")) {
          jsonFrame["soundID"] = SoundManager::GetID(jsonFrame["soundID"].asString());
        }
        
        // Convert eye shape/whichEye to enums:
        if(jsonFrame.isMember("whichEye")) {
          jsonFrame["whichEye"] = GetWhichEye(jsonFrame["whichEye"]);
        }
        if(jsonFrame.isMember("shape")) {
          jsonFrame["shape"] = GetEyeShape(jsonFrame["shape"]);
        }
        
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
      
    return RESULT_OK;
    
  } // DefineHardCodedAnimations()

} // namespace Cozmo
} // namespace Anki