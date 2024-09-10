//
//  StopPlaybackCmd.hpp
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 2/1/16.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#ifndef StopPlaybackCmd_hpp
#define StopPlaybackCmd_hpp

#include "mayaIncludes.h"


class StopPlaybackCmd : public MPxCommand {

public:
  
  static int jobNumber;
  
  const static char* callbackName;
  
  static std::function<void(void)> doItFunc;

  StopPlaybackCmd();
  
  virtual ~StopPlaybackCmd();
  
  virtual MStatus doIt(const MArgList&);
  
  static void* creator();
  
};

#endif /* StopPlaybackCmd_hpp */
