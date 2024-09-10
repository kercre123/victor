/***********************************************************************
The content of this file includes source code for the sound engine
portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
Two Source Code" as defined in the Source Code Addendum attached
with this file.  Any use of the Level Two Source Code shall be
subject to the terms and conditions outlined in the Source Code
Addendum and the End User License Agreement for Wwise(R).

Version: <VERSION>  Build: <BUILDNUMBER>
Copyright (c) <COPYRIGHTYEAR> Audiokinetic Inc.
***********************************************************************/

#pragma once

#include <AK/SoundEngine/Common/IAkPlugin.h>
#define AKALSASINK_MAXDEVICENAME_LENGTH 256
#define AKALSASINK_MAXCHANNEL 16
#define AKALSASINK_DEFAULT_NBCHAN 1
#define AKALSASINK_AVG_NBSAMPLE 48000
#define AKALSASINK_WAIT_TIMEOUT 1000
#define AKALSASINK_MIN_NB_OUTBUFFER 2

const unsigned long AKEFFECTID_ALSASINK = 169;
// Channel modes
enum ChannelModes
{
	CHANNELMODE_AUTODETECT = 0,
	CHANNELMODE_ANONYMOUS, 
	CHANNELMODE_CUSTOM,
	CHANNELMODE_MONO,
	CHANNELMODE_STEREO,
	CHANNELMODE_5_1,
	CHANNELMODE_7_1,
	CHANNELMODE_MAX_MODES	// Keep last
};

// Parameters IDs for the Wwise or RTPC.
enum AkAlsaSinkPluginParamIDs
{
	PARAMID_CHANNELMODE = 0,
	PARAMID_CHANNELINFO, // Mask or num channels in anonymous mode
	PARAMID_NUMBUFFERS,
	PARAMID_DEVICENAME,
	PARAMID_MASTERMODE,
	PARAMID_LAST //Keep Last
};

struct AkAlsaSinkParamStruct
{
	ChannelModes	eChannelMode;
	AkUInt32		uChannelInfo; // Mask or num channels in anonymous mode
	AkUInt32		uNumBuffers;
	AkOSChar		szDeviceName[AKALSASINK_MAXDEVICENAME_LENGTH];
	bool			bMasterMode;
};

class AkAlsaSinkPluginParams : public AK::IAkPluginParam
{
public:
	// Constructor/destructor.
	AkAlsaSinkPluginParams();
	~AkAlsaSinkPluginParams();
	AkAlsaSinkPluginParams(const AkAlsaSinkPluginParams & in_rCopy);

	// Create duplicate.
	IAkPluginParam * Clone(AK::IAkPluginMemAlloc * in_pAllocator);

	// Init/Term.
	AKRESULT Init(AK::IAkPluginMemAlloc *	in_pAllocator,
		const void *			in_pParamsBlock,
		AkUInt32				in_ulBlockSize
		);
	AKRESULT Term(AK::IAkPluginMemAlloc * in_pAllocator);

	// Blob set.
	AKRESULT SetParamsBlock(const void * in_pParamsBlock,
		AkUInt32 in_ulBlockSize
		);

	// Update one parameter.
	AKRESULT SetParam(AkPluginParamID in_ParamID,
		const void * in_pValue,
		AkUInt32 in_ulParamSize
		);

	const AkAlsaSinkParamStruct& GetParams()
	{
		return m_params;
	}

private:
	AkAlsaSinkParamStruct m_params;
};

