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
// StreamingWavePortal.cpp
//
// Streaming Wave Portal Wwise plugin: Main header file for the Audio Input DLL.
//
// This was code was adapted from Ak Audio Input sampe project
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StreamingWavePortal.h"
#include <AK/Wwise/Utilities.h>
#include "StreamingWavePortalPlugin.h"

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

#include <math.h>

// CStreamingWavePortal
BEGIN_MESSAGE_MAP(CStreamingWavePortal, CWinApp)
END_MESSAGE_MAP()


// CStreamingWavePortal construction
CStreamingWavePortal::CStreamingWavePortal()
{
}


// The one and only CStreamingWavePortal object
CStreamingWavePortal theApp;

// CStreamingWavePortal initialization
BOOL CStreamingWavePortal::InitInstance()
{
  __super::InitInstance();
	AK::Wwise::RegisterWwisePlugin();		
	return TRUE;
}

// CStreamingWavePortal exit code
int CStreamingWavePortal::ExitInstance()
{
	return 0;
}

/////////////// DLL exports ///////////////////

// Plugin creation
AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
	return new StreamingWavePortalPlugin();
}

/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;
DEFINE_PLUGIN_REGISTER_HOOK;
