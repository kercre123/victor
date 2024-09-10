// AkAlsaSinkPlugin.cpp  
//
#include "stdafx.h"
#include "AkAlsaSinkPlugin.h"
#include ".\Help\TopicAlias.h"
#include <string>

using namespace AK;
using namespace Wwise;

// Property names
static LPCWSTR szChannelMode = L"ChannelConfig";
static LPCWSTR szChannelInfo = L"ChannelInfo";
static LPCWSTR szNumBuffers= L"NumBuffers";
static LPCWSTR szDeviceName = _T("DeviceName");
static LPCWSTR szMasterMode = _T("MasterMode");

AkAlsaSinkPlugin::AkAlsaSinkPlugin() : m_pPSet(NULL)
{
}

AkAlsaSinkPlugin::~AkAlsaSinkPlugin()
{
}

void AkAlsaSinkPlugin::Destroy()
{
	delete this;
}

void AkAlsaSinkPlugin::SetPluginPropertySet(IPluginPropertySet * in_pPSet)
{
	m_pPSet = in_pPSet;
}

HINSTANCE AkAlsaSinkPlugin::GetResourceHandle() const
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

bool AkAlsaSinkPlugin::GetDialog(eDialog in_eDialog, UINT & out_uiDialogID, PopulateTableItem *& out_pTable) const
{
	// This sample does not provide UI, it will simply dispay properties in the default property editor.
	return false;
}

bool AkAlsaSinkPlugin::WindowProc(eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult)
{
	out_lResult = 0;
	return false;
}

bool AkAlsaSinkPlugin::GetBankParameters(const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter) const
{
	CComVariant varProp;
	m_pPSet->GetValue(in_guidPlatform, szChannelMode, varProp);
	in_pDataWriter->WriteInt32(varProp.intVal);
	m_pPSet->GetValue(in_guidPlatform, szChannelInfo, varProp);
	in_pDataWriter->WriteInt32(varProp.intVal);
	m_pPSet->GetValue(in_guidPlatform, szNumBuffers, varProp);
	in_pDataWriter->WriteInt32(varProp.intVal);
	m_pPSet->GetValue(in_guidPlatform, szDeviceName, varProp);
	in_pDataWriter->WriteUtf16String(varProp.bstrVal);
	m_pPSet->GetValue(in_guidPlatform, szMasterMode, varProp);
	in_pDataWriter->WriteBool(varProp.boolVal != 0);

	return true;
}


bool AkAlsaSinkPlugin::Help(HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode) const
{
	
	WCHAR currentExeFileName[MAX_PATH];
	

	//Get Current Wwise exe directory
	GetModuleFileName(NULL, currentExeFileName, MAX_PATH);

	//find the position where the directory name end.
	std::string::size_type directory_pos = std::wstring(currentExeFileName).find_last_of(_T("\\/"));

	//Plugins specific topic page
    WCHAR chmPreCmd[MAX_PATH] = L"mk:@MSITStore:";
	WCHAR chmPostCmd[MAX_PATH] = L"\\..\\..\\..\\Help\\WwiseAutomotive.chm::/automotive__plugins.html#ae_sink_alsa";
	std::wstring chmFinalCmd(chmPreCmd);

	//Compute Total char in order not to append a path name greater than MAX_PATH 260 on windows
	AkInt32 totalChar;
	totalChar = std::wcslen(chmPreCmd) + int(directory_pos) + std::wcslen(chmPostCmd);

	if (totalChar < MAX_PATH)
	{
		chmFinalCmd += std::wstring(currentExeFileName).substr(0, directory_pos);
		chmFinalCmd += std::wstring(chmPostCmd);

		HINSTANCE retval = ShellExecute(in_hWnd, _T("open"), _T("hh.exe"), chmFinalCmd.c_str(), NULL, SW_SHOW);		
		return true;
	}

	return false;	
}
