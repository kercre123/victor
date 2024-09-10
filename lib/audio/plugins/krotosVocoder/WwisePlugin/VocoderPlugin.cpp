#include "stdafx.h"
#include "resource.h"
#include "VocoderPlugin.h"
#include "KrotosVocoderFXFactory.h"
#include <AK/Tools/Common/AkAssert.h>

using namespace AK;
using namespace Wwise;

// Vocoder property names
static LPCWSTR szCarrier                = L"Carrier";
static LPCWSTR szCarrierFrequency       = L"CarrierFrequency";
static LPCWSTR szCarrierFrequencyMax    = L"CarrierFrequencyMax";
static LPCWSTR szCarrierFrequencyMin    = L"CarrierFrequencyMin";
static LPCWSTR szCarrierWaveform        = L"CarrierWaveform";
static LPCWSTR szEnvelopeAttackTime     = L"EnvelopeAttackTime";
static LPCWSTR szEnvelopeReleaseTime    = L"EnvelopeReleaseTime";
static LPCWSTR szPitchTrackerAlgorithm  = L"PitchTrackerAlgorithm";
static LPCWSTR szEQSlider1              = L"EQSlider1";
static LPCWSTR szEQSlider2              = L"EQSlider2";
static LPCWSTR szEQSlider3              = L"EQSlider3";
static LPCWSTR szEQSlider4              = L"EQSlider4";
static LPCWSTR szEQSlider5              = L"EQSlider5";
static LPCWSTR szEQSlider6              = L"EQSlider6";
static LPCWSTR szEQSlider7              = L"EQSlider7";
static LPCWSTR szEQSlider8              = L"EQSlider8";
static LPCWSTR szWetVol					= L"MasterLevel";
static LPCWSTR szDryVol					= L"DryLevel";

// Constructor
VocoderPlugin::VocoderPlugin()
    : m_pPSet( NULL ), m_hwndPropView( NULL )
{
}

// Destructor
VocoderPlugin::~VocoderPlugin()
{
}

// Implement the destruction of the Wwise source plugin.
void VocoderPlugin::Destroy()
{
    delete this;
}

// Set internal values of the property set (allow persistence)
void VocoderPlugin::SetPluginPropertySet( IPluginPropertySet* in_pPSet )
{
    m_pPSet = in_pPSet;
}

// Take necessary action on property changes. 
// Note: user also has the option of catching appropriate message in WindowProc function.
void VocoderPlugin::NotifyPropertyChanged(const GUID& in_guidPlatform, LPCWSTR in_szPropertyName)
{
    // Grey out redundant controls when carrier mode changes
    if (wcscmp(in_szPropertyName, szCarrier) == 0)
    {
        if (m_hwndPropView)
        {
            ConfigureUI();
        }
    }

    // Ensure carrier frequency maximum is >= minimum
    if (wcscmp(in_szPropertyName, szCarrierFrequencyMax) == 0)
    {
        CComVariant varPropMax;
        m_pPSet->GetValue(m_pPSet->GetCurrentPlatform(), szCarrierFrequencyMax, varPropMax);
        CComVariant varPropMin;
        m_pPSet->GetValue(m_pPSet->GetCurrentPlatform(), szCarrierFrequencyMin, varPropMin);

        if(varPropMax < varPropMin)
        {
            m_pPSet->SetValue(m_pPSet->GetCurrentPlatform(), szCarrierFrequencyMin, varPropMax);
        }
    }

    // Ensure carrier frequency minimum is <= maximum
    if (wcscmp(in_szPropertyName, szCarrierFrequencyMin) == 0)
    {
        CComVariant varPropMin;
        m_pPSet->GetValue(m_pPSet->GetCurrentPlatform(), szCarrierFrequencyMin, varPropMin);
        CComVariant varPropMax;
        m_pPSet->GetValue(m_pPSet->GetCurrentPlatform(), szCarrierFrequencyMax, varPropMax);

        if (varPropMax < varPropMin)
        {
            m_pPSet->SetValue(m_pPSet->GetCurrentPlatform(), szCarrierFrequencyMax, varPropMin);
        }
    }
}

// Get access to UI resource handle.
HINSTANCE VocoderPlugin::GetResourceHandle() const
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );
    return AfxGetStaticModuleState()->m_hCurrentResourceHandle;
}

// Set the property names to UI control binding populated table.
bool VocoderPlugin::GetDialog( eDialog in_eDialog, UINT& out_uiDialogID, PopulateTableItem*& out_pTable ) const
{
    AKASSERT( in_eDialog == SettingsDialog );

    out_uiDialogID = IDD_VOCODER_BIG;
    out_pTable = 0;

    return true;
}

// Standard window function, user can intercept what ever message that is of interest to him to implement UI behavior.
bool VocoderPlugin::WindowProc( eDialog in_eDialog, HWND in_hWnd, UINT in_message, WPARAM in_wParam, LPARAM in_lParam, LRESULT & out_lResult )
{
    switch ( in_message )
    {
    case WM_INITDIALOG:
        m_hwndPropView = in_hWnd;
        ConfigureUI();
        break;
    case WM_DESTROY:
        m_hwndPropView = NULL;
        break;
    }

    out_lResult = 0;
    return false;
}

// Store current plugin settings into banks when asked to.
bool VocoderPlugin::GetBankParameters( const GUID& in_guidPlatform, AK::Wwise::IWriteData* in_pDataWriter ) const
{
    CComVariant varProp;

    // Pack parameters in bank 
    // IMPORTANT NOTE: they need to be written and read on the AudioEngine side in the same order
    m_pPSet->GetValue( in_guidPlatform, szCarrier, varProp);
    in_pDataWriter->WriteInt32( varProp.intVal );
    m_pPSet->GetValue( in_guidPlatform, szCarrierFrequency, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szCarrierFrequencyMax, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szCarrierFrequencyMin, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szCarrierWaveform, varProp);
    in_pDataWriter->WriteInt32( varProp.intVal );
    m_pPSet->GetValue( in_guidPlatform, szEnvelopeAttackTime, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEnvelopeReleaseTime, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szPitchTrackerAlgorithm, varProp);
    in_pDataWriter->WriteInt32( varProp.intVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider1, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider2, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider3, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider4, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider5, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider6, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider7, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
    m_pPSet->GetValue( in_guidPlatform, szEQSlider8, varProp);
    in_pDataWriter->WriteReal32( varProp.fltVal );
	m_pPSet->GetValue(in_guidPlatform, szWetVol, varProp);
	in_pDataWriter->WriteReal32(varProp.fltVal);
	m_pPSet->GetValue(in_guidPlatform, szDryVol, varProp);
	in_pDataWriter->WriteReal32(varProp.fltVal);
    return true;
}

// Implement online help when the user clicks on the "?" icon .
bool VocoderPlugin::Help( HWND in_hWnd, eDialog in_eDialog, LPCWSTR in_szLanguageCode ) const
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() ) ;

    return false;
}

void VocoderPlugin::ConfigureUI()
{
    CComVariant varProp;
    HWND hwndItem;
    m_pPSet->GetValue(m_pPSet->GetCurrentPlatform(), szCarrier, varProp);

    // Carrier max frequency only usable for Vocoder::Carrier::OscillatorPitchTracking
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_MAX);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal == 1);
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_MAX_LABEL);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal == 1);

    // Carrier min frequency only usable for Vocoder::Carrier::OscillatorPitchTracking
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_MIN);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal == 1);
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_MIN_LABEL);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal == 1);

    // Carrier frequency only usable for Vocoder::Carrier::OscillatorFrequencySetByUI
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_FREQ);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal == 2);
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_FREQ_LABEL);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal == 2);

    // Carrier waveform not usable for Vocoder::Carrier::Noise
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_WAVE);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal != 3);
    hwndItem = GetDlgItem(m_hwndPropView, IDC_CARRIER_WAVE_LABEL);
    AKASSERT(hwndItem);
    ::EnableWindow(hwndItem, varProp.intVal != 3);
}
