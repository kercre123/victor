//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "hijackAudioPlugin.h"
#include "hijackAudioPluginId.h"
#include ".\Help\TopicAlias.h"
#include <AK/Tools/Common/AkAssert.h>


using namespace AK;
using namespace Wwise;

// Delay property names
static LPCWSTR szPluginId = L"PluginId";

// Bind non static text UI controls to properties for property view
AK_BEGIN_POPULATE_TABLE(DelayProp)
AK_POP_ITEM(IDC_EDIT1, szPluginId)
AK_END_POPULATE_TABLE()

// These IDs must be the same as those specified in the plug-in's XML definition file.
// Note that there are restrictions on the values you can use for CompanyID, and PluginID
// must be unique for the specified CompanyID. Furthermore, these IDs are persisted
// in project files. NEVER CHANGE THEM or existing projects will not recognize this Plug-in.
// Be sure to read the SDK documentation regarding Plug-ins XML definition files.
const short HijackAudioPlugin::CompanyID = AKCOMPANYID_ANKI;
const short HijackAudioPlugin::PluginID = AKEFFECTID_HIJACK;

// Constructor
HijackAudioPlugin::HijackAudioPlugin()
	: m_pPSet( NULL ), m_hwndPropView( NULL )
{
}

// Destructor
HijackAudioPlugin::~HijackAudioPlugin()
{
}

// Implement the destruction of the Wwise source plugin.
void HijackAudioPlugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void HijackAudioPlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Take necessary action on property changes. 
// Note: user also has the option of catching appropriate message in WindowProc function.
void HijackAudioPlugin::NotifyPropertyChanged( const GUID & in_guidPlatform, LPCWSTR in_szPropertyName )
{
	/*if ( !wcscmp( in_szPropertyName, szFeedbackEnabled ) )
	{
		if ( m_hwndPropView ) 
	}*/
}

// Get access to UI resource handle.
HINSTANCE HijackAudioPlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Set the property names to UI control binding populated table.
bool HijackAudioPlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	AKASSERT( in_eDialog == SettingsDialog );

	out_uiDialogID = IDD_DELAY_BIG;
	out_pTable = DelayProp;

	return true;
}

// Standard window function, user can intercept what ever message that is of interest to him to implement UI behavior.
bool HijackAudioPlugin::WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
	switch ( in_message )
	{
	case WM_INITDIALOG:
		m_hwndPropView = in_hWnd;
		break;
	case WM_DESTROY:
		m_hwndPropView = NULL;
		break;
	}

	out_lResult = 0;
	return false;
}

// Store current plugin settings into banks when asked to.
bool HijackAudioPlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
	CComVariant varProp;

	// Pack parameters in bank 
	// IMPORTANT NOTE: they need to be written and read on the AudioEngine side in the same order
	/*m_pPSet->GetValue( in_guidPlatform, szDelayTime, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );
	m_pPSet->GetValue( in_guidPlatform, szFeedback, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );
	m_pPSet->GetValue( in_guidPlatform, szWetDryMix, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );
	m_pPSet->GetValue( in_guidPlatform, szOutputLevel, varProp );
	in_pDataWriter->WriteReal32( varProp.fltVal );
	m_pPSet->GetValue( in_guidPlatform, szFeedbackEnabled, varProp );
	in_pDataWriter->WriteBool( varProp.boolVal != 0 );
	m_pPSet->GetValue( in_guidPlatform, szProcessLFE, varProp );
	in_pDataWriter->WriteBool( varProp.boolVal != 0 );*/
	m_pPSet->GetValue(in_guidPlatform, szPluginId, varProp);
	in_pDataWriter->WriteInt32(varProp.intVal);

    return true;
}

// Implement online help when the user clicks on the "?" icon .
bool HijackAudioPlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
	/*
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	if ( in_eDialog == AK::Wwise::IAudioPlugin::SettingsDialog )
		::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, ONLINEHELP::Delay_Properties, 0 );
	else
		return false;

	return true;
	*/

	return false;
}
