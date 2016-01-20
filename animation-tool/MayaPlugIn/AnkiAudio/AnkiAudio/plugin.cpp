//
//  plugin.cpp
//  AnkiAudio
//
//  Created by Molly Jameson on 1/19/16.
//    Copyright © 2016 Molly Jameson. All rights reserved.
//

//-----------------------------------------------------------------------------

#define OSMac_

#include <vector>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MEventMessage.h>
#include <maya/MAnimControl.h>
#include <maya/MItDependencyNodes.h>

/////////////////////////////////////////////////////////////////
//
// Plugin
//
/////////////////////////////////////////////////////////////////

// http://stackoverflow.com/questions/30568841/event-in-maya-api-to-capture-currenttime-frame-change
MCallbackIdArray g_CallbackIDs;

// TODO: move to it's own class when we know what goes in here.
struct RelevantMayaAudioInfo
{
  RelevantMayaAudioInfo( const char* name, double time )
  {
    m_Name = name;
    m_Time = time;
  }
  std::string m_Name;
  double      m_Time;
};
std::vector<RelevantMayaAudioInfo> g_AudioNodes;


// Our callback function thats called when scrubbed or played after the MEL command is triggered.
void onTimeChangedCallback(void* clientData)
{
  const MTime t = MAnimControl::currentTime();
  /*MString debugOut("Scrubbed ");
  debugOut += t.as(MTime::kMilliseconds);
  MGlobal::displayInfo( debugOut );*/
  
  // wwiseTODO: parse g_AudioNodes and test if we need to play anything
  
  return;
}

// data Could be wwise client of "this"
void enableNotifyOnTimelineTimeChange(void* data)
{
  MCallbackId callbackId = MEventMessage::addEventCallback("timeChanged", (MMessage::MBasicFunction) onTimeChangedCallback, &data);
  g_CallbackIDs.append(callbackId);
  
  // TODO: use attribute keyframes on node instead of empty audio nodes?
  // TODO: do this whenever audio is "dirtied" instead of just on export.
  // Builds the audio files from maya based on empty wavs
  MItDependencyNodes it(MFn::kDependencyNode);
  for (; !it.isDone(); it.next())
  {
    MFnDependencyNode fn(it.item());
    if (fn.typeName() == "audio")
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
    }
  }
}

void disableNotifyOnTimelineTimeChange()
{
  if (g_CallbackIDs.length() != 0)
  {
    // Remove the MEventMessage callback
    MEventMessage::removeCallbacks(g_CallbackIDs);
  }
  g_AudioNodes.clear();
}

class AnkiAudioMayaPlugin : public MPxCommand {
public:
  AnkiAudioMayaPlugin()
  {
    MGlobal::displayInfo("AnkiAudioMayaPlugin CONSTRUCTOR");
  };
  virtual ~AnkiAudioMayaPlugin()
  {
    MGlobal::displayInfo("AnkiAudioMayaPlugin DESTRUCTOR");
  };
  virtual MStatus doIt(const MArgList&);
  static void* creator();
};

void* AnkiAudioMayaPlugin::creator()
{
  return new AnkiAudioMayaPlugin();
}

// This is called when MEL command "AnkiAudioMaya" is actually called the first time.
// This class is created and destroyed immediately after
MStatus AnkiAudioMayaPlugin::doIt( const MArgList& args )
{
  MStatus stat = MS::kSuccess;
  
  // Since this class is derived off of MPxCommand, you can use the
  // inherited methods to return values and set error messages
  //
  MGlobal::displayInfo( "doIt!" );
  setResult( "AnkiAudioMayaPlugin command executed6" );
  
  // wwiseTODO: init global wise client here
  disableNotifyOnTimelineTimeChange();
  enableNotifyOnTimelineTimeChange(this);
  
  return stat;
}


/////////////////////////////////////////////////////////////////
//
// Global functions to actually initialize things. Called as soon
// as plugin is loaded/unloaded in maya
//
/////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
MStatus initializePlugin (MObject obj) {
	#pragma EXPORT
	MFnPlugin plugin (obj, "Anki", "0.0.1", "Any") ;

  MStatus		stat = plugin.registerCommand("AnkiAudioMaya", AnkiAudioMayaPlugin::creator);
  if (!stat)
    stat.perror("registerCommand");
  return stat;
}

MStatus uninitializePlugin (MObject obj) {
	#pragma EXPORT
	//- this method is called when the plug-in is unloaded from Maya. It 
	//- deregisters all of the services that it was providing.
	//-		obj - a handle to the plug-in object (use MFnPlugin to access it)
	MFnPlugin plugin (obj) ;

  disableNotifyOnTimelineTimeChange();
  
  MStatus stat = plugin.deregisterCommand("AnkiAudioMaya");
  if (!stat)
    stat.perror("deregisterCommand");
  return stat;
}


