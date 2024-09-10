//
//  setParameterCmd.cpp
//  AnkiMayaWWisePlugIn
//
//  Copyright © 2018 Anki, Inc. All rights reserved.
//

#include "pluginCommands/setParameterCmd.hpp"


const char* SetParameterCmd::mayaCommand = "AnkiMayaWWisePlugIn_SetParameter";

std::function<void(MString, float)> SetParameterCmd::doItFunc;


SetParameterCmd::SetParameterCmd()
{
  //    MGlobal::displayInfo("SetParameterCmd CONSTRUCTOR");
};

SetParameterCmd::~SetParameterCmd()
{
  //    MGlobal::displayInfo("SetParameterCmd DESTRUCTOR");
};

void* SetParameterCmd::creator()
{
  return new SetParameterCmd();
}

// This class is created and destroyed immediately after
MStatus SetParameterCmd::doIt( const MArgList& args )
{
  MStatus stat = MS::kSuccess;

  // Since this class is derived off of MPxCommand, you can use the
  // inherited methods to return values and set error messages

  if ( args.length() < 2 ) {
    stat = MS::kFailure;
  } else {
    MString paramName = args.asString( 0 );
    double paramValue = args.asDouble( 1 );
    if ( SetParameterCmd::doItFunc != nullptr ) {
      doItFunc(paramName, paramValue);
    }
  }

  return stat;
}

