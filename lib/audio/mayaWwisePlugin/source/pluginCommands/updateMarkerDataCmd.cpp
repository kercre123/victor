//
//  UpdateMarkerDataCmd.cpp
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 2/1/16.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#include "pluginCommands/updateMarkerDataCmd.hpp"


const char* UpdateMarkerDataCmd::mayaCommand = "AnkiMayaWWisePlugIn_UpdateEventData";

EventActionCapture      UpdateMarkerDataCmd::eventActionCap;
StateActionCapture      UpdateMarkerDataCmd::stateActionCap;
SwitchActionCapture     UpdateMarkerDataCmd::switchActionCap;
ParameterActionCapture  UpdateMarkerDataCmd::parameterActionCap;


std::function< void (void) > UpdateMarkerDataCmd::updateEventDataCompleteFunc;


AudioKeyFrameMap UpdateMarkerDataCmd::_keyframeMap;


UpdateMarkerDataCmd::UpdateMarkerDataCmd()
{
//  MGlobal::displayInfo("UpdateMarkerDataCmd CONSTRUCTOR");
};

UpdateMarkerDataCmd::~UpdateMarkerDataCmd()
{
//  MGlobal::displayInfo("UpdateMarkerDataCmd DESTRUCTOR");
};


void* UpdateMarkerDataCmd::creator()
{
  return new UpdateMarkerDataCmd();
}


// This class is created and destroyed immediately after
MStatus UpdateMarkerDataCmd::doIt( const MArgList& args )
{
  MStatus stat = MS::kSuccess;
  
  updateEventData(this);
  
  if (UpdateMarkerDataCmd::updateEventDataCompleteFunc != nullptr) {
    UpdateMarkerDataCmd::updateEventDataCompleteFunc();
  }
  
  return stat;
}

                  
void UpdateMarkerDataCmd::cleanUp()
{
  // Clean up old static data
  UpdateMarkerDataCmd::_keyframeMap.clear();
  eventActionCap.Reset();
  stateActionCap.Reset();
  switchActionCap.Reset();
  parameterActionCap.Reset();
}

void UpdateMarkerDataCmd::updateEventData(void* data)
{
  // Clean up old static data
  cleanUp();
  
  // TODO: use attribute keyframes on node instead of empty audio nodes?
  // TODO: do this whenever audio is "dirtied" instead of just on export.
  // Builds the audio files from maya based on empty wavs
  MItDependencyNodes it(MFn::kDependencyNode);
  
  for (; !it.isDone(); it.next())
  {
    MFnDependencyNode fn(it.item());
    
    // x:AnkiAudioNode
    // AnkiAudioNode_WwiseIdEnum
    // Prints out the audio as it is on trax
    /*if (fn.typeName() == "audio")
     {
     MStatus ret;
     const MPlug plug = fn.findPlug("offset", &ret);
     MTime timeOffset;
     ret = plug.getValue(timeOffset);
     g_AudioNodes.push_back(RelevantMayaAudioInfo(fn.name().asChar(),timeOffset.as(MTime::kMilliseconds)));
     MString debugOut("Added audio ");
     debugOut += fn.name();
     debugOut += " - ";
     debugOut += timeOffset.as(MTime::kMilliseconds);
     MGlobal::displayInfo(debugOut);
     }*/
  }
  
  // Query Types
  eventActionCap.QueryMaya();
  stateActionCap.QueryMaya();
  switchActionCap.QueryMaya();
  parameterActionCap.QueryMaya();
  
  // Add queries to static events
  eventActionCap.AddActionsToMap(UpdateMarkerDataCmd::_keyframeMap);
  stateActionCap.AddActionsToMap(UpdateMarkerDataCmd::_keyframeMap);
  switchActionCap.AddActionsToMap(UpdateMarkerDataCmd::_keyframeMap);
  parameterActionCap.AddActionsToMap(UpdateMarkerDataCmd::_keyframeMap);
  
}

const AudioKeyframe* UpdateMarkerDataCmd::FrameEvent(KeyframeTime frame)
{
  const auto it = UpdateMarkerDataCmd::_keyframeMap.find(frame);
  if ( it != UpdateMarkerDataCmd::_keyframeMap.end() ) {
    return &it->second;
  }
  return nullptr;
}


