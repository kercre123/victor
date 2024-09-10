//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/AudioPlugin.h>

class WavePortalPlugin
	: public AK::Wwise::DefaultAudioPluginImplementation
{
public:
	WavePortalPlugin();
	~WavePortalPlugin();

  // AK::Wwise::IPluginBase
  virtual void Destroy();

  // AK::Wwise::IAudioPlugin
  virtual void SetPluginPropertySet(AK::Wwise::IPluginPropertySet * in_pPSet);

  virtual HINSTANCE GetResourceHandle() const;
  virtual bool GetDialog(eDialog in_eDialog, UINT & out_uiDialogID, AK::Wwise::PopulateTableItem *& out_pTable) const;
  virtual bool GetBankParameters(const GUID & in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter) const;
  virtual bool DisplayNameForProp(LPCWSTR in_szPropertyName, LPWSTR out_szDisplayName, UINT in_unCharCount) const;
  virtual bool Help(HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode) const;

  static const short CompanyID;
  static const short PluginID;

private:
  AK::Wwise::IPluginPropertySet * m_pPSet;

};
