//
//  plugin.h
//  AnkiMayaWWisePlugIn
//
//  Created by Jordan Rivas on 2/1/16.
//  Copyright © 2016 Anki, Inc. All rights reserved.
//

#ifndef plugin_h
#define plugin_h

#define OSMac_
#include <maya/MFnPlugin.h>
#include "mayaIncludes.h"


#include <vector>
#include <functional>


void EnableTimeChangedCallbacks();
void DisableTimeChangedCallbacks();

MStatus UpdateMarkerDataCmd_Register(MFnPlugin& plugin);
MStatus UpdateMarkerDataCmd_Unregister(MFnPlugin& plugin);

MStatus StopPlaybackCmd_RegisterCommand(MFnPlugin& plugin);
MStatus StopPlaybackCmd_UnregisterCommand(MFnPlugin& plugin);

MStatus ReloadSoundBanksCmd_RegisterCommand(MFnPlugin& plugin);
MStatus ReloadSoundBanksCmd_UnregisterCommand(MFnPlugin& plugin);

MStatus PlayAudioEventCmd_RegisterCommand(MFnPlugin& plugin);
MStatus PlayAudioEventCmd_UnregisterCommand(MFnPlugin& plugin);

MStatus SetParameterCmd_RegisterCommand(MFnPlugin& plugin);
MStatus SetParameterCmd_UnregisterCommand(MFnPlugin& plugin);

bool LoadAudioSoundBanks();

void HandleTimeChanged();

bool PlayAudioEvent(const MString eventName);

bool SetParameterValue(const MString paramName, const float paramValue);


#endif /* plugin_h */
