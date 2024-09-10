/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

  Version: v2017.2.5  Build: 6619
  Copyright (c) 2006-2018 Audiokinetic Inc.
*******************************************************************************/
//////////////////////////////////////////////////////////////////////
//
// StreamingWavePortalPlugin.cpp
//
// Streaming Wave Portal plugin implementation.
//
// This was code was adapted from Ak Audio Input sampe project
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "StreamingWavePortalPlugin.h"
#include "Help\TopicAlias.h"


#define _USE_MATH_DEFINES
#include <math.h>

using namespace AK;
using namespace Wwise;

void StreamingWavePortalPlugin::GetFormatCallbackFunc(
    AkPlayingID		in_playingID,   ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
    AkAudioFormat&  io_AudioFormat  ///< Already filled format, modify it if required.
    )
{
}

void StreamingWavePortalPlugin::ExecuteCallbackFunc(
    AkPlayingID		in_playingID,  ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
    AkAudioBuffer*	io_pBufferOut  ///< Buffer to fill
    )
{
  /*
	AkSampleType * pSamples = io_pBufferOut->GetChannel( 0 );

	for ( int i = 0; i < io_pBufferOut->MaxFrames(); ++i )
		pSamples[ i ] = (float) sin( (double) i / 256.0 * M_PI * 2.0 );

	io_pBufferOut->eState = AK_DataReady;
	io_pBufferOut->uValidFrames = io_pBufferOut->MaxFrames();
  */
}

// Constructor
StreamingWavePortalPlugin::StreamingWavePortalPlugin()
: m_pPSet( NULL )
{
}

// Destructor
StreamingWavePortalPlugin::~StreamingWavePortalPlugin()
{
}

// Implement the destruction of the Wwise source plugin.
void StreamingWavePortalPlugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void StreamingWavePortalPlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Get access to UI resource handle.
HINSTANCE StreamingWavePortalPlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Determine what dialog just get called and set the property names to UI control binding populated table.
bool StreamingWavePortalPlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{

	switch ( in_eDialog )
	{
	case SettingsDialog:
		out_uiDialogID = IDD_StreamingWavePortalPlugin_BIG;
		out_pTable = NULL;
		return true;

	case ContentsEditorDialog:
		out_uiDialogID = IDD_StreamingWavePortalPlugin_SMALL;
		out_pTable = NULL;
		return true;
	}

	return false;
}

// Store current plugin settings into banks when asked to.
bool StreamingWavePortalPlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
	CComVariant varProp;

  m_pPSet->GetValue(in_guidPlatform, szStreamingWavePortalPluginId, varProp);
	in_pDataWriter->WriteInt32( varProp.intVal );

	return true;
}

// Implement online help when the user clicks on the "?" icon .
bool StreamingWavePortalPlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
  /*
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	DWORD dwTopic = ONLINEHELP::Audio_Input_Property;
	if ( in_eDialog == AK::Wwise::IAudioPlugin::ContentsEditorDialog )
		dwTopic = ONLINEHELP::Audio_Input_Contents;

	::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, dwTopic, 0 );
  */
	return false;
}
