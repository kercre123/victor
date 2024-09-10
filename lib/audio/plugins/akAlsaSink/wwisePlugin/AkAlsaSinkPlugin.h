#pragma once

#include <AK/Wwise/AudioPlugin.h>

class AkAlsaSinkPlugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	AkAlsaSinkPlugin();
	~AkAlsaSinkPlugin();

	// AK::Wwise::IPluginBase
	virtual void Destroy();

	// AK::Wwise::IAudioPlugin
	virtual void SetPluginPropertySet(AK::Wwise::IPluginPropertySet * in_pPSet);

	virtual HINSTANCE GetResourceHandle() const;
	virtual bool GetDialog(eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable) const;
	virtual bool WindowProc(eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult);

	virtual bool GetBankParameters(const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter) const;
	virtual bool Help(HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode) const;


private:

	AK::Wwise::IPluginPropertySet * m_pPSet;
};
