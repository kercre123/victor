// KrotosVocoder.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "KrotosVocoder.h"
#include "VocoderPlugin.h"
#include <AK/Wwise/Utilities.h>
#include <AK/Tools/Common/AkAssert.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//  Note!
//
//      If this DLL is dynamically linked against the MFC
//      DLLs, any functions exported from this DLL which
//      call into MFC must have the AFX_MANAGE_STATE macro
//      added at the very beginning of the function.
//
//      For example:
//
//      extern "C" BOOL PASCAL EXPORT ExportedFunction()
//      {
//          AFX_MANAGE_STATE(AfxGetStaticModuleState());
//          // normal function body here
//      }
//
//      It is very important that this macro appear in each
//      function, prior to any calls into MFC.  This means that
//      it must appear as the first statement within the 
//      function, even before any object variable declarations
//      as their constructors may generate calls into the MFC
//      DLL.
//
//      Please see MFC Technical Notes 33 and 58 for additional
//      details.
//

// CVocoderApp
BEGIN_MESSAGE_MAP(CVocoderApp, CWinApp)
END_MESSAGE_MAP()


// CVocoderApp construction

CVocoderApp::CVocoderApp()
{

}

// The one and only CVocoderApp object
CVocoderApp theApp;

// CVocoderApp initialization
BOOL CVocoderApp::InitInstance()
{
    CWinApp::InitInstance();
    AK::Wwise::RegisterWwisePlugin();
    return TRUE;
}

AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
    return new VocoderPlugin;
}


/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;
DEFINE_PLUGIN_REGISTER_HOOK;