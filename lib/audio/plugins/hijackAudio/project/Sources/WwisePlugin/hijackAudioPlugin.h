//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/AudioPlugin.h>

class HijackAudioPlugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	HijackAudioPlugin();
	~HijackAudioPlugin();

	// AK::Wwise::IPluginBase
	virtual void Destroy();

	// AK::Wwise::IAudioPlugin
	virtual void SetPluginPropertySet( AK::Wwise::IPluginPropertySet * in_pPSet );
	virtual void NotifyPropertyChanged( const GUID & in_guidPlatform, LPCWSTR in_szPropertyName );
	virtual HINSTANCE GetResourceHandle() const;
	virtual bool GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable ) const;
	virtual bool WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult );
    virtual bool GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const;
	virtual bool Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const;

	static const short CompanyID;
	static const short PluginID;

private:
	AK::Wwise::IPluginPropertySet * m_pPSet;
	HWND m_hwndPropView;
};
