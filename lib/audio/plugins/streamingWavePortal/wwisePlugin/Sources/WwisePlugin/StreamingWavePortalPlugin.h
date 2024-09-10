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
// StreamingWavePortalPlugin.h
//
// Streaming Wave Portal plugin implementation.
//
// This was code was adapted from Ak Audio Input sampe project
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/AudioPlugin.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>

// Streaming Wave Portal plugin property names
static LPCTSTR szStreamingWavePortalPluginId = L"StreamingWavePortalPluginId";

class StreamingWavePortalPlugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	StreamingWavePortalPlugin();
	~StreamingWavePortalPlugin();

	// AK::Wwise::IPluginBase
	virtual void Destroy();

	// AK::Wwise::IAudioPlugin
	virtual void SetPluginPropertySet( AK::Wwise::IPluginPropertySet * in_pPSet );

	virtual HINSTANCE GetResourceHandle() const;
	virtual bool GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable ) const;

  virtual bool GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const;

	virtual bool Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const;

	static void GetFormatCallbackFunc(
		AkPlayingID		in_playingID,   ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
		AkAudioFormat&  io_AudioFormat  ///< Already filled format, modify it if required.
		);

	static void ExecuteCallbackFunc(
		AkPlayingID		in_playingID,  ///< Playing ID (same that was returned from the PostEvent call or from the PlayAudioInput call.
		AkAudioBuffer*	io_pBufferOut  ///< Buffer to fill
	    );

private:
	//SoundInput m_Input;
	AK::Wwise::IPluginPropertySet * m_pPSet;
};
