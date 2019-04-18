/*	
 * Acapela TTS Bundling
 *
 * Copyright (c) 2010 Acapela Group SA. All rights reserved.
 * 
 * This file and the accompanying libraires and files can only be used if you 
 * have signed an agreement with Acapela Group SA. 
 * 
 * Redistribution of the source code and the library files is stricly 
 * prohibited. The client is responsible for keeping his client ID and 
 * password secret.
 * 
 * The written agreement signed with Acapela Group SA supersedes this notice 
 * of use.
 */

/* 
 * Here are the steps to bundle a license in a client application:
 * 
 * 1. Include this header file
 * 2. Create an instance of CTTSBundle:
 *    
 *    CTTSBundle bundle(CTTSBundle::BUNDLE_BABTTS);
 * 
 * 4. Use the created CTTSBundle instance:
 *    
 *    bundle.SetLicense();
 */

#if !defined(_BABTTSBUNDLE_H_)
#define _BABTTSBUNDLE_H_

#if !defined(WIN32)
	#include <dlfcn.h>
	#include <limits.h>
#endif

#if defined(WIN32)
	#define LOADSYMBOL GetProcAddress 
	#define CLOSELIB FreeLibrary			
	typedef HMODULE	DYNHANDLE;
#else
	#if !defined(LOADSYMBOL)
		#define LOADSYMBOL dlsym
	#endif
	
	#if !defined(CLOSELIB )
		#define CLOSELIB dlclose
	#endif
	
	typedef void*	DYNHANDLE;
#endif

static DYNHANDLE BabTtsBundleDLL=NULL;

#if defined(WIN32)
	#define DECL_PrivBabTTS_SetLicense 95 
#else
	#define DECL_PrivBabTTS_SetLicense "PrivBabTTS_SetLicense"
#endif

#if defined(WIN64) || defined(_WIN64)
	#define BABTTSBUNDLE_ACATTS_LIB_PREFIX ".\\"
	#define BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME "AcaTTS.64.dll"
#elif defined(WIN32) || defined(_WIN32)
	#define BABTTSBUNDLE_ACATTS_LIB_PREFIX ".\\"
	#define BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME "AcaTTS.dll"
#elif defined(__APPLE__)
	#define BABTTSBUNDLE_ACATTS_LIB_PREFIX "./"
	#define BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME "libacatts.dylib"
#else 
	#define BABTTSBUNDLE_ACATTS_LIB_PREFIX "./"
	#if defined(__LP64__)
		#define BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME "AcaTTS.64.so"
	#else
		#define BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME "AcaTTS.so"
	#endif
#endif

#if !defined(BABTTS_LIB)
	#define BABTTS_LIB \
		_T(BABTTSBUNDLE_ACATTS_LIB_PREFIX) _T(BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME)
#endif
	
#if !defined(BABTTS_LIBNAME)
	#define BABTTS_LIBNAME \
		_T(BABTTSBUNDLE_ACATTS_LIB_NAKEDNAME)
#endif

typedef int (WINAPI *pPrivBabTTS_SetLicense)(const char* lpszLic);

class CTTSBundle
{
public:
	typedef enum { BUNDLE_BABTTS =20 } BundleID;
	CTTSBundle(BundleID bundleid=BUNDLE_BABTTS)
	{
		TCHAR szTemp[512];
#if defined(WIN32)
		BabTtsBundleDLL=GetModuleHandle(BABTTS_LIB);
		if (BabTtsBundleDLL!=NULL)
		{
			// We found BABTTS_LIB --> get the full name.
			if (0==GetModuleFileName(BabTtsBundleDLL,szTemp,512))
			{
				_tcscpy(szTemp,BABTTS_LIB);
			}
		}
		else
		{
			_tcscpy(szTemp,BABTTS_LIB);
		}
#else
		Dl_info infoDylib;
		if (BabTTS_Open!=NULL)
		{
			if(dladdr((void*)BabTTS_Open, &infoDylib))
			{
				strcpy(szTemp, infoDylib.dli_fname);
			}
		}
		else
		{
			if(dladdr("", &infoDylib))
			{
				char *lastseparator;
				strcpy(szTemp, infoDylib.dli_fname);
				lastseparator = strrchr(szTemp, '/');
				szTemp[lastseparator-szTemp+1] = '\0';
				strcat(szTemp, BABTTS_LIB);
			}
			else
				strcpy(szTemp,BABTTS_LIB);
		}
#endif
		Init(bundleid,szTemp);
	}
	CTTSBundle(BundleID bundleid,LPCTSTR lpszEngine)
	{
		Init(bundleid,lpszEngine);
	}
	~CTTSBundle()
	{
		if (BabTtsBundleDLL)
		{
			CLOSELIB(BabTtsBundleDLL);
			BabTtsBundleDLL=0;
		}
	
	}
	bool Register(unsigned long dwClientKey, unsigned long dwPassword)
	{
		(void) dwClientKey; // Unused parameter.
		(void) dwPassword; // Unused parameter.
		return true;//for backward compatibility only
	}
	bool SetLicense()
	{
		const char* myLic="MYLICENSEHERE";
		return SetLicense(myLic);
	}
	
protected:
	pPrivBabTTS_SetLicense PrivBabTTS_SetLicense;
	bool SetLicense(const char* lpszLicense)
	{
		if (NULL==PrivBabTTS_SetLicense || NULL==lpszLicense)
			return false;
		int ret=PrivBabTTS_SetLicense(lpszLicense);
		if (ret)
			return true;
		else
			return false;
	}
	void Init(BundleID bundleid,LPCTSTR lpszLib)
	{
		(void) bundleid; // Unused parameter.
		PrivBabTTS_SetLicense = NULL;
		
#if defined(WIN32)
		TCHAR buffer[MAX_PATH];
		LPCTSTR path = lpszLib;
		LPCTSTR separator = _T("");
		DWORD attributes = 0;
		
		if ('\0' == lpszLib[0])
		{
			path = BABTTS_LIB;
		}
		else if (
			INVALID_FILE_ATTRIBUTES == (
				attributes = GetFileAttributes(path)
			)
		)
		{
			return;
		}
		else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ('\\' != lpszLib[_tcslen(lpszLib) - 1])
			{
				separator = _T("\\");
			}
			
			if (
				_sntprintf(
					buffer, MAX_PATH - 1, _T("%s%s%s"), lpszLib, separator, 
					BABTTS_LIB
				) >= (MAX_PATH - 1)
			)
			{
				return;
			}
			
			path = buffer;
		}
		
		BabTtsBundleDLL = LoadLibrary(path);
		
		if ((NULL == BabTtsBundleDLL) || (BabTtsBundleDLL < (HMODULE) 32))
		{
			return;
		}
#else
		char buffer[PATH_MAX];
		const char *path = (const char *) lpszLib;
		const char *separator = "";
		struct stat statBuffer;
		
		if ('\0' == lpszLib[0])
		{
			path = BABTTS_LIB;
		}
		else if (-1 == stat(path, &statBuffer))
		{
			return;
		}
		else if (S_ISDIR(statBuffer.st_mode))
		{
			if ('/' != lpszLib[strlen(lpszLib) - 1])
			{
				separator = "/";
			}
			
			if (
				snprintf(
					buffer, PATH_MAX - 1, "%s%s%s", lpszLib, separator, BABTTS_LIB
				) >= (PATH_MAX - 1)
			)
			{
				return;
			}
			
			path = buffer;
		}
		
		BabTtsBundleDLL = dlopen(path, RTLD_NOW);
		
		if (NULL == BabTtsBundleDLL) 
		{
			return;
		}
#endif
		
		PrivBabTTS_SetLicense = (pPrivBabTTS_SetLicense) LOADSYMBOL(BabTtsBundleDLL,(const char*) DECL_PrivBabTTS_SetLicense);
		
		if (NULL != PrivBabTTS_SetLicense)
		{
			return;
		}
		
#ifdef WIN32
		::MessageBox(NULL,_T("Incorrect Version of BabTts"),_T("Error"),MB_OK);
#else
		printf("Incorrect Version of BabTts\n");
#endif			
		CLOSELIB(BabTtsBundleDLL);
		
		return;
	}
};
#endif
