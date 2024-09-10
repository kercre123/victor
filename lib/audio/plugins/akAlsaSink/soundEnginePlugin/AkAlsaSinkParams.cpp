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

#include "AkAlsaSinkParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkAutoHelper.h"


// Constructor/destructor.
AkAlsaSinkPluginParams::AkAlsaSinkPluginParams()
{
}

AkAlsaSinkPluginParams::~AkAlsaSinkPluginParams()
{
}

// Copy constructor.
AkAlsaSinkPluginParams::AkAlsaSinkPluginParams(const AkAlsaSinkPluginParams & in_rCopy)
{
	m_params = in_rCopy.m_params;
}

// Create duplicate.
AK::IAkPluginParam * AkAlsaSinkPluginParams::Clone(AK::IAkPluginMemAlloc * in_pAllocator)
{
	AKASSERT(in_pAllocator != NULL);
	return AK_PLUGIN_NEW(in_pAllocator, AkAlsaSinkPluginParams(*this));
}

// Init/Term.
AKRESULT AkAlsaSinkPluginParams::Init(AK::IAkPluginMemAlloc *	in_pAllocator,
	const void *			in_pParamsBlock,
	AkUInt32				in_ulBlockSize)
{
	if (in_ulBlockSize == 0)
	{
		// Init default parameters.
		memset(&m_params, 0, sizeof(m_params));
		m_params.eChannelMode = CHANNELMODE_AUTODETECT;
		m_params.uChannelInfo = 0;
		m_params.uNumBuffers = 2;
		m_params.bMasterMode = true;
		return AK_Success;
	}

	return SetParamsBlock(in_pParamsBlock, in_ulBlockSize);
}

AKRESULT AkAlsaSinkPluginParams::Term(AK::IAkPluginMemAlloc * in_pAllocator)
{
	AKASSERT(in_pAllocator != NULL);

	AK_PLUGIN_DELETE(in_pAllocator, this);
	return AK_Success;
}

// Blob set.
AKRESULT AkAlsaSinkPluginParams::SetParamsBlock(const void * in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
	AKRESULT eResult = AK_Success;

	AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
	m_params.eChannelMode = (ChannelModes)READBANKDATA(AkInt32, pParamsBlock, in_ulBlockSize);
	m_params.uChannelInfo = READBANKDATA(AkUInt32, pParamsBlock, in_ulBlockSize);
	m_params.uNumBuffers = READBANKDATA(AkUInt32, pParamsBlock, in_ulBlockSize);
	AkUInt32 uDeviceNameLength = AKPLATFORM::AkUtf16StrLen((AkUtf16*)pParamsBlock);
	eResult = ConvertUTF16ToChar(m_params.szDeviceName, pParamsBlock, sizeof(m_params.szDeviceName), uDeviceNameLength);
	if(eResult != AK_Success)
	{
		return eResult;
	}
	SKIPBANKBYTES((AkUInt32) (AKPLATFORM::OsStrLen(m_params.szDeviceName) + 1) * sizeof(AkUtf16), pParamsBlock, in_ulBlockSize);
	m_params.bMasterMode = READBANKDATA(bool, pParamsBlock, in_ulBlockSize);
	CHECKBANKDATASIZE(in_ulBlockSize, eResult);

	return eResult;
}

// Update one parameter.
AKRESULT AkAlsaSinkPluginParams::SetParam(AkPluginParamID in_ParamID,
	const void * in_pValue,
	AkUInt32 in_ulParamSize)
{
	AKASSERT(in_pValue != NULL);
	if (in_pValue == NULL)
	{
		return AK_InvalidParameter;
	}
	AKRESULT eResult = AK_Success;

	switch (in_ParamID)
	{
	case PARAMID_CHANNELMODE:
		m_params.eChannelMode = (ChannelModes) (*(AkInt32*) (in_pValue));
		break;
	case PARAMID_CHANNELINFO:
		m_params.uChannelInfo = (*(AkUInt32*) (in_pValue));
		break;
	case PARAMID_NUMBUFFERS:
		m_params.uNumBuffers = (*(AkUInt32*) (in_pValue));
		break;
	case PARAMID_DEVICENAME:
		{
			AkUInt32 uDeviceNameLength = AKPLATFORM::AkUtf16StrLen((AkUtf16*)in_pValue);
			eResult = ConvertUTF16ToChar(m_params.szDeviceName, (AkUInt8 *)in_pValue, sizeof(m_params.szDeviceName), uDeviceNameLength);
			if(eResult != AK_Success)
			{
				return eResult;
			}
		}
		break;
	case PARAMID_MASTERMODE:
		m_params.bMasterMode = (*(bool*) (in_pValue));
		break;
	default:
		AKASSERT(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	return eResult;
}
