//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "wavePortalPlugin.h"
#include ".\Help\TopicAlias.h"
#include <AK/Tools/Common/AkAssert.h>
#include "wavePortalPluginId.h"


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
const short WavePortalPlugin::CompanyID = AKCOMPANYID_ANKI;
const short WavePortalPlugin::PluginID = AKSOURCEID_WAVEPORTAL;

// Constructor
WavePortalPlugin::WavePortalPlugin()
	: m_pPSet( NULL ) //, m_hwndPropView( NULL )
{
}

// Destructor
WavePortalPlugin::~WavePortalPlugin()
{
}

// Implement the destruction of the Wwise source plugin.
void WavePortalPlugin::Destroy()
{
	delete this;
}

// Set internal values of the property set (allow persistence)
void WavePortalPlugin::SetPluginPropertySet( IPluginPropertySet * in_pPSet )
{
	m_pPSet = in_pPSet;
}

// Get access to UI resource handle.
HINSTANCE WavePortalPlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Set the property names to UI control binding populated table.
bool WavePortalPlugin::GetDialog( eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable ) const
{
	AKASSERT( in_eDialog == SettingsDialog );

	out_uiDialogID = IDC_EDIT1;
	out_pTable = DelayProp;

	return true;
}

// Store current plugin settings into banks when asked to.
bool WavePortalPlugin::GetBankParameters( const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
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


// Allow Wwise to retrieve a user friendly name for that property (e.g. Undo etc.).
bool WavePortalPlugin::DisplayNameForProp(LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount) const
{
  /*
  for (DisplayNameInfo * pDisplayNameInfo = g_DisplayNames; pDisplayNameInfo->wszPropName; pDisplayNameInfo++)
  {
    if (!wcscmp(in_szPropertyName, pDisplayNameInfo->wszPropName))
    {
      // Get resource handle
      HINSTANCE hInst = AfxGetStaticModuleState()->m_hCurrentResourceHandle;
      ::LoadString(hInst, pDisplayNameInfo->uiDisplayName, out_szDisplayName, in_unCharCount);
      return true;
    }
  }

  AKASSERT(!"Need display name for property");
  */
  return false;
}

// Implement online help when the user clicks on the "?" icon .
bool WavePortalPlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
	AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

	if ( in_eDialog == AK::Wwise::IAudioPlugin::SettingsDialog )
		::SendMessage( in_hWnd, WM_AK_PRIVATE_SHOW_HELP_TOPIC, ONLINEHELP::Delay_Properties, 0 );
	else
		return false;

	return true;
}

