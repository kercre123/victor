//
//  setParameterCmd.hpp
//  AnkiMayaWWisePlugIn
//
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#ifndef setParameterCmd_hpp
#define setParameterCmd_hpp

#define OSMac_
#include <maya/MArgList.h>
#include "mayaIncludes.h"


class SetParameterCmd : public MPxCommand {
  
public:
  
  static const char* mayaCommand;
  
  static std::function<void(MString, float)> doItFunc;
  
  SetParameterCmd();
  
  virtual ~SetParameterCmd();
  
  virtual MStatus doIt(const MArgList&);
  
  static void* creator();
  
};


#endif /* setParameterCmd_hpp */
