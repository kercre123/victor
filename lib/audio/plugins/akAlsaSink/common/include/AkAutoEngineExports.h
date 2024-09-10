//////////////////////////////////////////////////////////////////////
//
// AkAutoEngineExports.h
//
// Export/import DLL macro.
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/AkPlatforms.h>

/// \file 
/// Export/import directives.

#if defined(AK_WIN)
#ifdef AKAUTOENGINEDLL_EXPORTS
	/// Automotive engine API function declaration for DLL import/export
	#define AKAUTOENGINEDLL_API __declspec(dllexport)
#else
	/// Automotive engine API function declaration for DLL import/export
	#define AKAUTOENGINEDLL_API __declspec(dllimport)
#endif
#elif defined(AK_LINUX)
    #ifdef AKAUTOENGINEDLL_EXPORTS
	/// Automotive engine API function declaration for import/export
        #define AKAUTOENGINEDLL_API __attribute__ ((visibility ("default")))
    #else
	#define AKAUTOENGINEDLL_API
    #endif  
#endif

