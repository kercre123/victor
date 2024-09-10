//
//  playAudioEventCmd.hpp
//  AnkiMayaWWisePlugIn
//
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#ifndef playAudioEventCmd_hpp
#define playAudioEventCmd_hpp

#define OSMac_
#include <maya/MArgList.h>
#include "mayaIncludes.h"


class PlayAudioEventCmd : public MPxCommand {
  
public:
  
  static const char* mayaCommand;
  
  static std::function<void(MString)> doItFunc;
  
  PlayAudioEventCmd();
  
  virtual ~PlayAudioEventCmd();
  
  virtual MStatus doIt(const MArgList&);
  
  static void* creator();
  
};


#endif /* playAudioEventCmd_hpp */
