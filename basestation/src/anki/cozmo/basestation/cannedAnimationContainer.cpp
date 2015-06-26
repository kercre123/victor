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

#include "anki/cozmo/basestation/cannedAnimationContainer.h"

#include "anki/util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
  CannedAnimationContainer::CannedAnimationContainer()
  {
    DefineHardCoded();
  }
  
  Result CannedAnimationContainer::AddAnimation(const std::string& name)
  {
    Result lastResult = RESULT_OK;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      const s32 animID = static_cast<s32>(_animations.size());
      _animations[name].first = animID;
      
      PRINT_NAMED_INFO("CannedAnimationContainer.AddAnimation",
                       "Adding new animation named '%s' with ID=%d\n",
                       name.c_str(), animID);
      
    } else {
      PRINT_NAMED_ERROR("CannedAnimationContainer.AddAnimation.DuplicateName",
                        "Animation named '%s' already exists. Not adding.\n",
                        name.c_str());
      lastResult = RESULT_FAIL;
    }
    
    return lastResult;
  }

  Animation* CannedAnimationContainer::GetAnimation(const std::string& name)
  {
    Animation* animPtr = nullptr;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetAnimation.InvalidName",
                        "Animation requested for unknown animation '%s'.\n",
                        name.c_str());
    } else {
      animPtr = &retVal->second.second;
    }
    
    return animPtr;
  }

  AnimationID_t CannedAnimationContainer::GetID(const std::string& name) const
  {
    AnimationID_t animID = INVALID_ANIMATION_ID;
    
    auto retVal = _animations.find(name);
    if(retVal == _animations.end()) {
      PRINT_NAMED_ERROR("CannedAnimationContainer.GetID.InvalidName",
                        "ID requested for unknown animation '%s'.\n",
                        name.c_str());
    } else {
      animID = retVal->second.first;
    }
    
    return animID;
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
  
  /*
  Result CannedAnimationContainer::Send(RobotID_t robotID, IRobotMessageHandler* msgHandler)
  {
    Result lastResult = RESULT_OK;
    
    PRINT_STREAM_INFO("CannedAnimationContainer.Send",
                      "Sending " << _animations.size() << " animations to robot " << robotID << ".");
    
    for(auto & cannedAnimationByName : _animations)
    {
      const std::string&  name         = cannedAnimationByName.first;
      const s32           animID       = cannedAnimationByName.second.first;
      const Animation&    keyFrameList = cannedAnimationByName.second.second;
      
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
        if(msgHandler != nullptr) { // could be in a test environment
          msgHandler->SendMessage(robotID, clearMsg);
        }
        
        PRINT_NAMED_INFO("CannedAnimationContainer.Send",
                         "Sending '%s' animation definition with ID=%d.\n",
                         name.c_str(), animID);

        // Now send all the keyframe definition messages for this animation ID
        for(auto message : keyFrameList.GetMessages())
        {
          if(message != nullptr) {
            if(msgHandler != nullptr) { // could be in a test environment
              lastResult = msgHandler->SendMessage(robotID, *message);
            }
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
  */
  
  /*
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
  
  static f32 GetHeight(const Json::Value& json)
  {
    static const std::map<std::string, f32> LUT = {
      {"LOW_DOCK",  LIFT_HEIGHT_LOWDOCK},
      {"HIGH_DOCK", LIFT_HEIGHT_HIGHDOCK},
      {"CARRY",     LIFT_HEIGHT_CARRY},
    };
    
    const f32 DEFAULT = LIFT_HEIGHT_LOWDOCK;
    
    if(json.isString()) {
      auto result = LUT.find(json.asString());
      if(result == LUT.end()) {
        PRINT_STREAM_WARNING("GetHeight", "Unknown height " << json.toStyledString() << ", returning default.\n");
        return DEFAULT;
      } else {
        return result->second;
      }
    } else if(json.isNumeric()) {
      return json.asFloat();
    } else {
      PRINT_NAMED_WARNING("GetHeight",
                          "Encountered unknown height, returning default.\n");
      return DEFAULT;
    }
  } // GetHeight()

  static void GetColor(Json::Value& json)
  {
    if(json.isString()) {
      Json::Value temp; // can't seem to turn input json into array directly
      const ColorRGBA& color( NamedColors::GetByString(json.asString()));
      temp[0] = color.r();
      temp[1] = color.g();
      temp[2] = color.b();
      json = temp;
    } else if(!json.isArray()) {
      PRINT_NAMED_WARNING("GetColor", "Not sure how to translate color, not changing.\n");
    }
    
  } // GetColor()
  */
  
  Result CannedAnimationContainer::DefineFromJson(Json::Value& jsonRoot)
  {
    
    Json::Value::Members animationNames = jsonRoot.getMemberNames();
    
    /*
    // Add _all_ the animations first to register the IDs, so Trigger keyframes
    // which specify another animation's name will still work (because that
    // name should already exist, no matter the order the Json is parsed)
    for(auto const& animationName : animationNames) {
      AddAnimation(animationName);
    }
     */
    
    for(auto const& animationName : animationNames)
    {
      if(RESULT_OK != AddAnimation(animationName)) {
        PRINT_NAMED_INFO("CannedAnimationContainer.DefineFromJson.ReplaceName",
                          "Replacing existing animation named '%s'.\n",
                          animationName.c_str());
      }
      
      Animation* animation = GetAnimation(animationName);
      if(animation == nullptr) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Could not GetAnimation named '%s'.",
                          animationName.c_str());
        return RESULT_FAIL;
      }
      
      Result lastResult = animation->DefineFromJson(animationName,
                                                    jsonRoot[animationName]);
      
      // Sanity check
      if(animation->GetName() != animationName) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Animation's internal name ('%s') doesn't match container's name for it ('%s').\n",
                          animation->GetName().c_str(),
                          animationName.c_str());
        return RESULT_FAIL;
      }
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson",
                          "Failed to define animation '%s' from Json.\n",
                          animationName.c_str());
        return lastResult;
      }
      
      /*
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
        
        if(!jsonFrame.isMember("transitionIn")) {
          PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.NoTransitionIn",
                            "Missing 'transitionIn' field for '%s' frame of '%s' animation.\n",
                            jsonFrame["Name"].asString().c_str(),
                            animationName.c_str());
          return RESULT_FAIL;
        }
        
        if(!jsonFrame.isMember("transitionOut")) {
          PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.NoTransitionOut",
                            "Missing 'transitionOut' field for '%s' frame of '%s' animation.\n",
                            jsonFrame["Name"].asString().c_str(),
                            animationName.c_str());
          return RESULT_FAIL;
        }
        
        if(jsonFrame.isMember("animToPlay")) {
          if(!jsonFrame["animToPlay"].isString()) {
            PRINT_NAMED_ERROR("CannedAnimationContainer.DefineFromJson.animToPlayString",
                              "Expecting 'animToPlay' field for '%s' frame of '%s' animation to be a string.\n",
                              jsonFrame["Name"].asString().c_str(),
                              animationName.c_str());
          } else {
            jsonFrame["animToPlay"] = GetID(jsonFrame["animToPlay"].asString());
          }
        }
        
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
        
        // Convert named lift heights to values
        if(jsonFrame.isMember("height_mm")) {
          jsonFrame["height_mm"]     = GetHeight(jsonFrame["height_mm"]);
        } else if(jsonFrame.isMember("lowHeight_mm")) {
          jsonFrame["lowHeight_mm"]  = GetHeight(jsonFrame["lowHeight_mm"]);
        } else if(jsonFrame.isMember("highHeight_mm")) {
          jsonFrame["highHeight_mm"] = GetHeight(jsonFrame["highHeight_mm"]);
        }
        
        // Convert color strings if necessary (this modifies the json value directly)
        if(jsonFrame.isMember("color")) {
          GetColor(jsonFrame["color"]);
        }
        
        RobotMessage* kfMessage = RobotMessage::CreateFromJson(jsonRoot[animationName][iFrame]); 
        if(kfMessage != nullptr) {
          KeyFrameList* keyFrames = GetKeyFrameList(animationName);
          if(keyFrames == nullptr) {
            return RESULT_FAIL;
          }
          keyFrames->AddKeyFrame(kfMessage);
        }
        
      } // for each frame
       */
    } // for each animation
    
    return RESULT_OK;
  } // CannedAnimationContainer::DefineFromJson()
  
  Result CannedAnimationContainer::DefineHardCoded()
  {
    /*
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
      startNodMsg->lowAngle_deg  = -25;
      startNodMsg->highAngle_deg =  25;
      startNodMsg->period_ms = 1200;
      keyFrames->AddKeyFrame(startNodMsg);
      
      // Stop the nod
      MessageAddAnimKeyFrame_StopHeadNod* stopNodMsg = new MessageAddAnimKeyFrame_StopHeadNod();
      stopNodMsg->animationID = animID;
      stopNodMsg->relTime_ms = 2400;
      stopNodMsg->finalAngle_deg = 0;
      keyFrames->AddKeyFrame(stopNodMsg);
    } // SLOW HEAD NOD
    */
    return RESULT_OK;
    
  } // DefineHardCodedAnimations()
  
  void CannedAnimationContainer::Clear()
  {
    _animations.clear();
  } // Clear()

} // namespace Cozmo
} // namespace Anki