/*
 * Acapela TTS Interface
 *
 * Copyright (c) 2010 Acapela Group SA. All rights reserved.
 * 
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS 
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN AGREEMENT OR 
 * ROYALTY FEES.
 */

/*
 * Here are the steps you need to follow to use this file to dynamically 
 * import AcaTts.dll/AcaTts.64.dll (Windows), libacatts.dylib (Mac OS X - 
 * Universal Binary) or AcaTTS.so/AcaTTS.64.so (Linux):
 * 
 * 1. Make sure that C-compatible header files ioBabTts.h and ifBabTtsDyn.h 
 *    are in the header file search path of your project.
 * 
 * 2. In any source file using the SDK functions:
 *    
 *    Windows-specific: #include <windows.h> (or <stdafx.h>)
 *    #include "ioBabTts.h"
 *    #include "ifBabTtsDyn.h"
 * 
 * 3. You need to implement the body of the dynamic library loading helper 
 *    functions (which are BabTtsInitDll(), BabTtsInitDllEx() and 
 *    BabTtsUninitDll()) in ONLY ONE source file:
 *    
 *    Windows-specific: #include <windows.h> (or <stdafx.h>)
 *    #include "ioBabTts.h"
 *    #define _BABTTSDYN_IMPL_
 *    #include "ifBabTtsDyn.h"
 *    #undef _BABTTSDYN_IMPL_
 * 
 * 4. Before using any of SDK functions, you will need to make sure that the 
 *    dynamic library is loaded and its functions are resolved, this can be 
 *    done in two different ways, by calling 
 *    
 *    - either BabTtsInitDll() if AcaTts.dll/AcaTts.64.dll, libacatts.dylib or 
 *      AcaTTS.so/AcaTTS.64.so lies in the current working directory (or 
 *      is available through the system dynamic library loader search path)
 *    
 *    - or BabTtsInitDllEx(LPCTSTR lpszLib) if lpszLib is either the 
 *      absolute (full) path to the dynamic library, including the file name, 
 *      (i.e., on Windows: "C:\Acapela Group\Acapela Multimedia\AcaTts.dll" or 
 *      "C:\Acapela Group\Acapela Multimedia\AcaTts.64.dll" for a 64-bit 
 *      process) or to the directory where the dynamic library lies, 
 *      terminated or not with a trailing path component separator (i.e., 
 *      "C:\Acapela Group\Acapela Multimedia\" or 
 *      "C:\Acapela Group\Acapela Multimedia")
 * 
 * 5. Once you are done with the TTS, unload the dynamic library by calling 
 *    BabTtsUninitDll() which also ensures proper dynamic library-related 
 *    clean up.
 */

#if !defined(_IFBABTTSDYN_H)
#define _IFBABTTSDYN_H

#if !defined(WINAPI)
	#if defined(WIN32) || defined(_WIN32)
		#error "WINAPI is not defined. Did you mess your header files order or forget to include windows.h or stdafx.h?"
	#else
		#define WINAPI
	#endif
#endif

#if defined(WIN32) || defined(_WIN32)
	#define _IFBABTTSDYN_T(str) _T(str)
	
	#define _IFBABTTSDYN_setLoadErrorStr(str) \
		_tcsncpy( \
			_IFBABTTSDYN_loadErrorStr, (str), ( \
				( \
					sizeof(_IFBABTTSDYN_loadErrorStr) / \
					sizeof(_IFBABTTSDYN_loadErrorStr[0u]) \
				) - 1u \
			) \
		)
	#define _IFBABTTSDYN_setLoadErrorStrF(fmt, ...) \
		_sntprintf( \
			_IFBABTTSDYN_loadErrorStr, ( \
				sizeof(_IFBABTTSDYN_loadErrorStr) / \
				sizeof(_IFBABTTSDYN_loadErrorStr[0u]) \
			), fmt, __VA_ARGS__ \
		)
	#define _CRT_SECURE_NO_WARNINGS
#else
	#define _IFBABTTSDYN_T(str) str
	#define _IFBABTTSDYN_setLoadErrorStr(str) \
		strncpy( \
			_IFBABTTSDYN_loadErrorStr, (str), ( \
				sizeof(_IFBABTTSDYN_loadErrorStr) / \
				sizeof(_IFBABTTSDYN_loadErrorStr[0u]) \
			) \
		)
	#define _IFBABTTSDYN_setLoadErrorStrF(fmt, ...) \
		snprintf( \
			_IFBABTTSDYN_loadErrorStr, ( \
				sizeof(_IFBABTTSDYN_loadErrorStr) / \
				sizeof(_IFBABTTSDYN_loadErrorStr[0u]) \
			), fmt, __VA_ARGS__ \
		)
#endif

/* API functions. */
#define _IFBABTTSDYN_API_FUNCS \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Init, bool, \
		(void) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Uninit, bool, \
		(void) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetNumVoices, long int, \
		(void) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_EnumVoices, BabTtsError, \
		(DWORD dwIndex, char szVoice[50]) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetVoiceInfo, BabTtsError, \
		(const char *lpszVoice, BABTTSINFO *lpInfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetVoiceInfoExImpl, BabTtsError, \
		(const char *lpszVoice, BABTTSINFO2 *lpInfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetInstanceInfo, BabTtsError, \
		(LPBABTTS lpObject, BABTTSINFO *lpInfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetInstanceInfoExImpl, BabTtsError, \
		(LPBABTTS lpObject, BABTTSINFO2 *lpInfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetVoiceLicense, BabTtsError, \
		(const char *lpszVoice, BABTTSLICINFO *lpInfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_InsertText, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszText, DWORD dwTextFlag) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_ReadBuffer, BabTtsError, \
		(LPBABTTS lpObject, void *lpBuffer, DWORD dwBufSize, DWORD *lpdwGen) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Create, LPBABTTS, \
		(void) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Open, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszSpeechFont, DWORD dwOpenFlag) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Close, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Speak, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszText, DWORD dwSpeakFlag) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Write, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszText, const char *lpszFileName, DWORD dwWriteFlag) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Reset, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_ResetSettings, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_SetCallback, BabTtsError, \
		(LPBABTTS lpObject, void *lpCallback, DWORD dwCallbackType) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetCallback, BabTtsError, \
		(LPBABTTS lpObject, void **lppCallback, DWORD *lpdwCallbackType) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_SetSettings, BabTtsError, \
		(LPBABTTS lpObject, BabTtsParam paramtype, DWORD_PTR dwParam) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetSettings, BabTtsError, \
		(LPBABTTS lpObject, BabTtsParam paramtype, DWORD_PTR *pdwParam) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_SetFilterPreset, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszPreset) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetFilterPreset, BabTtsError, \
		(LPBABTTS lpObject, char szPreset[50]) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Dialog, BabTtsError, \
		(LPBABTTS lpObject, HWND hWnd, BabTtsDlg dlgType) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Pause, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_Resume, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictLoad, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT *lppDict, const char *lpszFileName) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetNumLoaded, long int, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetMax, long int, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictEnum, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT *lppDict, DWORD dwIndex) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictIsModified, long int, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictUnload, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictUnloadAll, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictEnable, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictDisable, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictSave, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszFileName) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictSaveAll, BabTtsError, \
		(LPBABTTS lpObject) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictAddWord, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszWord, const char *lpszTrans, BabTtsWordType Type) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictRemoveWord, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszWord) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetWordTrans, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszWord, char *lpszTrans, DWORD *lpdwSize, BabTtsWordType *lpWordType) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetNumEntries, long int, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetEntry, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, DWORD dwIndex, char *lpszEntry, DWORD *lpdwEntrySize) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictAddWordEx, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszWord, const char *lpszTrans, BabTtsWordType Type, BabTtsWordEncoding Encoding) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictRemoveWordEx, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszWord,BabTtsWordEncoding Encoding) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetWordTransEx, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszWord, char *lpszTrans, DWORD *lpdwTransSize, BabTtsWordType *lpWordType, BabTtsWordEncoding Encoding) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetEntryEx, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, DWORD dwIndex, char *lpszEntry, DWORD *lpdwEntrySize, BabTtsWordEncoding Encoding) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetInfo, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, LPBABTTSDICTINFO lpDictinfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetInfoFromFile, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszFileName, LPBABTTSDICTINFO lpDictinfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictSetInfo, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, BABTTSDICTINFO Dictinfo) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictSetPassword, BabTtsError, \
		(LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszStr1, const char *lpszStr2) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictGetDefList, BabTtsError, \
		(LPBABTTS lpObject, char *lpszFileList, DWORD *lpdwSize) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictSetDefList, BabTtsError, \
		(LPBABTTS lpObject, const char *lpszFileList) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictImport, BabTtsError, \
		( \
			LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszBuffer, \
			BabTTSDictProc lpCallback, void *lpCallbackUserData, \
			DWORD dwFlags \
		) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_DictExport, BabTtsError, \
		( \
			LPBABTTS lpObject, LPBABTTSDICT lpDict, const char *lpszBuffer, \
			DWORD dwFlags \
		) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_PhoGetMouth, BabTtsError, \
		(LPBABTTS lpObject, DWORD_PTR dwPhoneme, BABTTSMOUTH *lpMouth) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_PhoGetDuration, BabTtsError, \
		(LPBABTTS lpObject, DWORD_PTR dwPhoneme, DWORD *lpdwDuration) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_PhoGetViseme, BabTtsError, \
		(LPBABTTS lpObject, DWORD_PTR dwPhoneme, DWORD *lpdwViseme) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetErrorNameImpl, const char *, \
		(BabTtsError dwErrorCode) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetErrorWideNameImpl, const wchar_t *, \
		(BabTtsError dwErrorCode) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_ExpandVoiceTemplate, BabTtsError, ( \
			const char *lpszVoiceName, const char *lpszTemplate, \
			char *lpBuffer, DWORD *lpdwBufferSize \
		) \
	) \
	_IFBABTTSDYN_API_FUNC( \
		BabTTS_GetPhonemeList, BabTtsError, ( \
			const char *const lpszVoiceName, \
			const BABTTSPHOINFO **const lpPhonemeList, DWORD *const lpdwNumPho \
		) \
	)

#include <stdio.h>

#if defined(WIN32) || defined(_WIN32)
	#define _IFBABTTSDYN_RESOLVE_SYMBOL GetProcAddress
	#define _IFBABTTSDYN_CLOSE_LIB FreeLibrary
	typedef HMODULE	DYNHANDLE;
	#include <tchar.h>
#else
	#if !defined(HWND)
		#define HWND void*
	#endif
	
	#if !defined(HMODULE)
		#define HMODULE void* 
	#endif
	
	#include <dlfcn.h>
	#include <unistd.h>
	
		#include <stdlib.h>
	
		#include <string.h>
	
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <limits.h>
	#define _IFBABTTSDYN_RESOLVE_SYMBOL dlsym 
	#define _IFBABTTSDYN_CLOSE_LIB dlclose
	typedef char	TCHAR;
	typedef const char*	LPCTSTR;
	typedef void*	DYNHANDLE;
#endif

#if defined(WIN32) || defined(_WIN32)
	#define _IFBABTTSDYN_ACATTS_LIB_DIRPATH ""
	
	#if defined(WIN64) || defined(_WIN64)
		#define _IFBABTTSDYN_ACATTS_LIB_NAKEDNAME "AcaTTS.64.dll"
	#else
		#define _IFBABTTSDYN_ACATTS_LIB_NAKEDNAME "AcaTTS.dll"
	#endif
#elif defined(__APPLE__)
	#define _IFBABTTSDYN_ACATTS_LIB_DIRPATH ""
	#define _IFBABTTSDYN_ACATTS_LIB_NAKEDNAME "libacatts.dylib"
#else 
	#define _IFBABTTSDYN_ACATTS_LIB_DIRPATH "./"
	
	#if defined(__LP64__)
		#define _IFBABTTSDYN_ACATTS_LIB_NAKEDNAME "AcaTTS.64.so"
	#else
		#define _IFBABTTSDYN_ACATTS_LIB_NAKEDNAME "AcaTTS.so"
	#endif
#endif

#if !defined(BABTTS_LIB)
	#define BABTTS_LIB \
		_IFBABTTSDYN_T(_IFBABTTSDYN_ACATTS_LIB_DIRPATH) _IFBABTTSDYN_T(_IFBABTTSDYN_ACATTS_LIB_NAKEDNAME)
#endif
	
#if !defined(BABTTS_LIBNAME)
	#define BABTTS_LIBNAME \
		_IFBABTTSDYN_T(_IFBABTTSDYN_ACATTS_LIB_NAKEDNAME)
#endif

#if defined(__cplusplus)
	extern "C"
	{
#endif /* __cplusplus */

/* Generate the pointer type declaration for each API function. */
#define _IFBABTTSDYN_API_FUNC(name, resultType, args) \
	typedef resultType (WINAPI *LP_##name) args;

_IFBABTTSDYN_API_FUNCS

#undef _IFBABTTSDYN_API_FUNC

#if defined(__cplusplus)
	}
#endif /* __cplusplus */

#if defined(_BABTTSDYN_IMPL_)
	static HMODULE _IFBABTTSDYN_hMod = NULL;
	static TCHAR _IFBABTTSDYN_loadErrorStr[192u] = _IFBABTTSDYN_T("");
	
	/* Generate the pointer declaration for each API function. */	
	#define _IFBABTTSDYN_API_FUNC(name, resultType, args) \
		LP_##name name = NULL;
	
	_IFBABTTSDYN_API_FUNCS
	
	#undef _IFBABTTSDYN_API_FUNC
	
	#if defined(WIN32) || defined(_WIN32)
		static const TCHAR *_IFBABTTSDYN_getLastWin32ErrorStr()
		{
			static TCHAR lastErrorStr[256u] = { _T('\0') };
			DWORD lastErrorCode = GetLastError(); 
			TCHAR buffer[256u] = { _T('\0') };
			
			memset(buffer, 0u, sizeof(buffer) / sizeof(buffer[0u]));
			
			if (
				0 == FormatMessage(
					(
						FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS
					), NULL, lastErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
					buffer, 
					((sizeof(buffer) / sizeof(buffer[0u])) - sizeof(TCHAR)), NULL
				)
			)
			{
				_sntprintf(
					lastErrorStr, 
					(sizeof(lastErrorStr) / sizeof(lastErrorStr[0u])), 
					_T("<unknown error> {%ld}"), lastErrorCode
				);
			}
			else
			{
				if (
					(_T('\0') != buffer[0u]) && 
					(_T(' ') == buffer[_tcslen(buffer) - sizeof(TCHAR)])
				)
				{
					buffer[_tcslen(buffer) - sizeof(TCHAR)] = _T('\0');
				}
				
				_sntprintf(
					lastErrorStr, (sizeof(lastErrorStr) / sizeof(lastErrorStr[0u])), 
					_T("%s {%ld}"), buffer, lastErrorCode
				);
			}
			
			return lastErrorStr;
		}
	#endif
	
	/* Forward declaration of function `BabTtsUninitDll()`, used by 
	   `_IFBABTTSDYN_loadFunctions()`. */
	void BabTtsUninitDll();
	
	static int _IFBABTTSDYN_loadFunctions(HMODULE hMod)
	{
		_IFBABTTSDYN_hMod = hMod;
		
		/* Generate the pointer symbol address resolution code for each API 
		   function.
		   
		   Note: If a symbol can't be resolved, symbol resolution is aborted, 
		   `BabTtsUninitDll()` is called and 
			`BabTtsInitDll()`/`BabTtsInitDllEx()` will return `NULL`. Therefore, 
			in that case, the AcaTTS module is properly unloaded and all the 
			function pointers of the API are properly reset to `NULL`. */
		#if defined(WIN32) || defined(_WIN32)
			#define _IFBABTTSDYN_API_FUNC(name, resultType, args) \
				if ( \
					NULL == ( \
						name = (LP_##name) _IFBABTTSDYN_RESOLVE_SYMBOL(hMod, #name) \
					) \
				) \
				{ \
					_IFBABTTSDYN_setLoadErrorStrF( \
						_T("couldn't resolve the address of symbol \"") \
						_T(#name) _T("\": %s"), _IFBABTTSDYN_getLastWin32ErrorStr() \
					); \
					BabTtsUninitDll(); \
					return 0; \
				}
		#else
			#define _IFBABTTSDYN_API_FUNC(name, resultType, args) \
				if ( \
					NULL == ( \
						name = (LP_##name) _IFBABTTSDYN_RESOLVE_SYMBOL(hMod, #name) \
					) \
				) \
				{ \
					_IFBABTTSDYN_setLoadErrorStrF( \
						"couldn't resolve the address of symbol \"" #name "\": " \
						"%s", dlerror() \
					); \
					BabTtsUninitDll(); \
					return 0; \
				}
		#endif
		
		_IFBABTTSDYN_API_FUNCS
		
		#undef _IFBABTTSDYN_API_FUNC
		
		return 1;
	}
	
	HMODULE BabTtsInitDllEx(LPCTSTR lpszLib)
	{
		HMODULE hMod;
		#if defined(WIN32) || defined(_WIN32)
			TCHAR buffer[MAX_PATH];
			LPCTSTR path = lpszLib;
			LPCTSTR separator = _T("");
			DWORD attributes = 0;
		#else
			char buffer[PATH_MAX];
			const char *path = (const char *) lpszLib;
			const char *separator = "";
			struct stat statBuffer;
		#endif
		
		_IFBABTTSDYN_setLoadErrorStr(_IFBABTTSDYN_T("(none)"));
		
		#if defined(WIN32) || defined(_WIN32)
			if (_T('\0') == lpszLib[0u])
			{
				path = BABTTS_LIB;
			}
			else if (
				(
					INVALID_FILE_ATTRIBUTES != 
					(attributes = GetFileAttributes(path))
				) && (attributes & FILE_ATTRIBUTE_DIRECTORY)
			)
			{
				if (_T('\\') != lpszLib[_tcslen(lpszLib) - 1])
				{
					separator = _T("\\");
				}
				
				if (
					_sntprintf(
						buffer, MAX_PATH - 1, _T("%s%s%s"), lpszLib, separator, 
						BABTTS_LIBNAME
					) >= (MAX_PATH - 1)
				)
				{
					_IFBABTTSDYN_setLoadErrorStr(
						_T("not enough space for expanding AcaTTS' specified ") 
						_T("directory path into a fully specified file path")
					);
					return NULL;
				}
				
				path = buffer;
			}
			
			hMod = LoadLibrary(path);
			
			if ((NULL == hMod) || (hMod < ((HMODULE) 32u)))
			{
				_IFBABTTSDYN_setLoadErrorStrF(
					_T("LoadLibrary() failed: %s"), _IFBABTTSDYN_getLastWin32ErrorStr()
				);
				return NULL;
			}
		#else
			if ('\0' == lpszLib[0])
			{
				path = BABTTS_LIB;
			}
			else if (
				(-1 != stat(path, &statBuffer)) && (S_ISDIR(statBuffer.st_mode))
			)
			{
				if ('/' != lpszLib[strlen(lpszLib) - 1])
				{
					separator = "/";
				}
				
				if (
					snprintf(
						buffer, PATH_MAX - 1, "%s%s%s", lpszLib, separator, 
						BABTTS_LIBNAME
					) >= (PATH_MAX - 1)
				)
				{
					_IFBABTTSDYN_setLoadErrorStr(
						"not enough space for expanding AcaTTS' specified " 
						"directory path into a fully specified file path"
					);
					return NULL;
				}
				
				path = buffer;
			}
			
			hMod = dlopen(path, RTLD_NOW);
			
			if (NULL == hMod) 
			{
				_IFBABTTSDYN_setLoadErrorStrF("dlopen() failed: %s", dlerror());
				return NULL;
			}
			
			setenv("ACATTSDYNLIBPATH", path, 1);
		#endif
		
		if (!_IFBABTTSDYN_loadFunctions(hMod))
		{
			return NULL;
		}
		
		return hMod;
	}

	HMODULE BabTtsInitDll()
	{
		HMODULE hMod;
		TCHAR szTemp[512u] = _IFBABTTSDYN_T("");
		
		_IFBABTTSDYN_setLoadErrorStr(_IFBABTTSDYN_T("(none)"));
		
		#if defined(WIN32) || defined(_WIN32)
			hMod = GetModuleHandle(BABTTS_LIB);
			
			if (
				(NULL == hMod) || (
					0u == GetModuleFileName(
						hMod, szTemp, (sizeof(szTemp) / sizeof(szTemp[0u]))
					)
				)
			)
			{
				_tcscpy(szTemp, BABTTS_LIB);
				hMod = LoadLibrary(szTemp);
				
				if ((NULL == hMod) || (hMod < ((HMODULE) 32u)))
				{
					_IFBABTTSDYN_setLoadErrorStrF(
						_T("LoadLibrary() failed: %s"), 
						_IFBABTTSDYN_getLastWin32ErrorStr()
					);
					return NULL;
				}
			}
		#else
			const char *const acattsDLibPath = getenv("ACATTS_DLIBPATH");
			
			if (NULL == acattsDLibPath)
			{
				{
					Dl_info infoDylib;
					
					if (0 == dladdr("", &infoDylib))
					{
						_IFBABTTSDYN_setLoadErrorStr("dladdr() failed");
						return NULL;
					}
					
					strncpy(
						szTemp, infoDylib.dli_fname, 
						((sizeof(szTemp) / sizeof(szTemp[0u])) - 1u)
					);
				}
				
				{
					const char *lastSeparator = strrchr(szTemp, '/');
					
					if (NULL != lastSeparator)
					{
						szTemp[lastSeparator - szTemp + 1] = '\0';
						strcat(szTemp, BABTTS_LIB);
					}
				}
			}
			else
			{
				strcpy(szTemp, acattsDLibPath);
			}
			
			hMod = dlopen(szTemp, RTLD_NOW);
			
			if (NULL == hMod) 
			{
				_IFBABTTSDYN_setLoadErrorStrF("dlopen() failed: %s", dlerror());
				return NULL;
			}
		#endif
		
		if (!_IFBABTTSDYN_loadFunctions(hMod))
		{
			return NULL;
		}
		
		return hMod;
	}
	
	void BabTtsUninitDll()
	{
		if (NULL != _IFBABTTSDYN_hMod)
		{
			_IFBABTTSDYN_CLOSE_LIB(_IFBABTTSDYN_hMod);
			_IFBABTTSDYN_hMod = NULL;
		}
		
		/* Generate the function pointer cleaning code for each API 
		   function. */	
		#define _IFBABTTSDYN_API_FUNC(name, resultType, args) \
			name = NULL;
		
		_IFBABTTSDYN_API_FUNCS
		
		#undef _IFBABTTSDYN_API_FUNC
	}
	
	inline const TCHAR *BabTtsGetInitDllErrorStr()
	{
		return _IFBABTTSDYN_loadErrorStr;
	}
#else
	/* Generate the pointer extern declaration for each API function. */	
	#define _IFBABTTSDYN_API_FUNC(name, resultType, args) \
		extern LP_##name name;
	
	_IFBABTTSDYN_API_FUNCS
	
	#undef _IFBABTTSDYN_API_FUNC
	
	extern HMODULE BabTtsInitDll();
	extern HMODULE BabTtsInitDllEx(LPCTSTR lpszLib);
	extern void BabTtsUninitDll();
	extern const char *BabTtsGetInitDllErrorStr();
	extern const wchar_t *BabTtsGetInitDllErrorWideStr();
#endif

/* Inline proxy function for `BabTTS_GetVoiceInfoExImpl()` which is the real 
   function implementation. */	
inline BabTtsError BabTTS_GetVoiceInfoEx(
	const char *lpszVoice, BABTTSINFO2 *lpInfo
)
{
	if (NULL != lpInfo)
	{
		lpInfo->dwStructSize = sizeof(BABTTSINFO2);
		lpInfo->dwStructVersion = BABTTSINFO_VERSION2;
	}
	
	return BabTTS_GetVoiceInfoExImpl(lpszVoice, lpInfo);
}

/* Inline proxy function for `BabTTS_GetInstanceInfoExImpl()` which is the 
   real function implementation. */	
inline BabTtsError BabTTS_GetInstanceInfoEx(
	LPBABTTS lpObject, BABTTSINFO2 *lpInfo
)
{
	if (NULL != lpInfo)
	{
		lpInfo->dwStructSize = sizeof(BABTTSINFO2);
		lpInfo->dwStructVersion = BABTTSINFO_VERSION2;
	}
	
	return BabTTS_GetInstanceInfoExImpl(lpObject, lpInfo);
}

/* Inline proxy function for `BabTTS_GetErrorNameImpl()` and 
   `BabTTS_GetErrorWideNameImpl()` which are the real function 
   implementations. */	
inline 
#if (defined(WIN32) || defined(_WIN32)) && defined(_UNICODE)
	const wchar_t *
#else
	const char *
#endif
BabTTS_GetErrorName(BabTtsError dwErrorCode)
{
	return 
		#if (defined(WIN32) || defined(_WIN32)) && defined(_UNICODE)
			BabTTS_GetErrorWideNameImpl
		#else
			BabTTS_GetErrorNameImpl
		#endif
	(dwErrorCode);
}

#undef _IFBABTTSDYN_CLOSE_LIB
#undef _IFBABTTSDYN_RESOLVE_SYMBOL
#undef _IFBABTTSDYN_API_FUNCS
#undef _IFBABTTSDYN_setLoadErrorStrF
#undef _IFBABTTSDYN_setLoadErrorStr

#endif /* _IF_IFBABTTSDYN_H */
