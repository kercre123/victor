//
//  StopPlaybackCmd.cpp
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 2/1/16.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#include "pluginCommands/stopPlaybackCmd.hpp"

int StopPlaybackCmd::jobNumber = 0;
const char* StopPlaybackCmd::callbackName = "StopPlaybackCmd";
std::function<void(void)> StopPlaybackCmd::doItFunc;


StopPlaybackCmd::StopPlaybackCmd()
{
  //    MGlobal::displayInfo("StopPlaybackCmd CONSTRUCTOR");
};

StopPlaybackCmd::~StopPlaybackCmd()
{
  //    MGlobal::displayInfo("StopPlaybackCmd DESTRUCTOR");
};

void* StopPlaybackCmd::creator()
{
  return new StopPlaybackCmd();
}

// This class is created and destroyed immediately after
MStatus StopPlaybackCmd::doIt( const MArgList& args )
{
  MStatus stat = MS::kSuccess;
  
  // Since this class is derived off of MPxCommand, you can use the
  // inherited methods to return values and set error messages
  
  if ( StopPlaybackCmd::doItFunc != nullptr ) {
    doItFunc();
  }
  
  return stat;
}
