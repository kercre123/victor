//
//  playAudioEventCmd.cpp
//  AnkiMayaWWisePlugIn
//
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#include "pluginCommands/playAudioEventCmd.hpp"


const char* PlayAudioEventCmd::mayaCommand = "AnkiMayaWWisePlugIn_PlayAudioEvent";

std::function<void(MString)> PlayAudioEventCmd::doItFunc;


PlayAudioEventCmd::PlayAudioEventCmd()
{
  //    MGlobal::displayInfo("PlayAudioEventCmd CONSTRUCTOR");
};

PlayAudioEventCmd::~PlayAudioEventCmd()
{
  //    MGlobal::displayInfo("PlayAudioEventCmd DESTRUCTOR");
};

void* PlayAudioEventCmd::creator()
{
  return new PlayAudioEventCmd();
}

// This class is created and destroyed immediately after
MStatus PlayAudioEventCmd::doIt( const MArgList& args )
{
  MStatus stat = MS::kSuccess;

  // Since this class is derived off of MPxCommand, you can use the
  // inherited methods to return values and set error messages

  for ( int i = 0; i < args.length(); i++ ) {
    MString arg = args.asString( i );
    if ( PlayAudioEventCmd::doItFunc != nullptr ) {
      doItFunc(arg);
    }
  }

  return stat;
}

