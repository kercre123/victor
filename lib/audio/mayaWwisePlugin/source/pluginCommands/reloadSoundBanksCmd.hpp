//
//  reloadSoundBanksCmd.hpp
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 2/4/16.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#ifndef reloadSoundBanksCmd_hpp
#define reloadSoundBanksCmd_hpp

#include "mayaIncludes.h"


class ReloadSoundBanksCmd : public MPxCommand {
  
public:
  
  static const char* mayaCommand;
  
  static std::function<void(void)> doItFunc;
  
  ReloadSoundBanksCmd();
  
  virtual ~ReloadSoundBanksCmd();
  
  virtual MStatus doIt(const MArgList&);
  
  static void* creator();
  
};


#endif /* reloadSoundBanksCmd_hpp */
