//
//  UpdateMarkerDataCmd.hpp
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 2/1/16.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#define VARIANT_ATTR_SUFFIX_START_INDEX 2

#ifndef UpdateMarkerDataCmd_hpp
#define UpdateMarkerDataCmd_hpp

#include "mayaIncludes.h"
#include "audioActionTypes.h"


#include <unordered_map>

#include <map>

#include <sstream>
#include <string>
#include <vector>

class UpdateMarkerDataCmd : public MPxCommand {
  
public:
  
  static EventActionCapture      eventActionCap;
  static StateActionCapture      stateActionCap;
  static SwitchActionCapture     switchActionCap;
  static ParameterActionCapture  parameterActionCap;
  
  static const char* mayaCommand;
  
  
  // This is called after the MayaCommand has been called and the event data has been updated
  static std::function<void(void)> updateEventDataCompleteFunc;
  
  
  UpdateMarkerDataCmd();
  
  virtual ~UpdateMarkerDataCmd();
  
  virtual MStatus doIt(const MArgList&);
  
  static void* creator();
  
  void cleanUp();
  
  static const AudioKeyframe* FrameEvent(KeyframeTime frame);
  
  
private:
  
  void updateEventData(void* data);
  
  // Key: time_ms
  // Val: Audio Keyframe
  static AudioKeyFrameMap _keyframeMap;
  
};


#endif /* UpdateMarkerDataCmd_hpp */
