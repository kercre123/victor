#ifndef __BB_BABILE__IF_BABILE_H__
#define __BB_BABILE__IF_BABILE_H__

#if defined(BABILE) || !defined( PALMOS5)
#ifdef __cplusplus
extern "C"
{
#endif
BBT_IMPORT_ BBT_EXPORT_ BB_S16		 BABILE_numAlloc(void);

BBT_IMPORT_ BBT_EXPORT_ BB_S16		 BABILE_alloc( BABILE_MemParam* babileParams,BB_MemRec* memTab);

BBT_IMPORT_ BBT_EXPORT_ BB_TCHAR*	 BABILE_getVersion(void);

BBT_IMPORT_ BBT_EXPORT_ BB_TCHAR*	 BABILE_getVersionEx(BABILE_Obj* lpBabil,BB_TCHAR* psVer, BB_S16 size);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_changeSpeechFont(BABILE_Obj* bab, BB_DbLs* SpeechFont);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_getError(BABILE_Obj* bab, BB_ERROR* mbrError, BB_ERROR* nlpError);

BBT_IMPORT_ BBT_EXPORT_ BB_S32		 BABILE_readText(BABILE_Obj* bab,BB_TCHAR* lpszText, BB_S16* lpBuffer, BB_U32 nbWanted,BB_U32* samples);

BBT_IMPORT_ BBT_EXPORT_ BB_S32		 BABILE_textToPhoStr(BABILE_Obj* bab,BB_TCHAR* lpszText, BABILE_PhoMode dwPhoMode, BB_TCHAR* lpBuffer, BB_S32 nbWanted,BB_U32 *samples);

BBT_IMPORT_ BBT_EXPORT_ BB_S32		 BABILE_readPhoStr(BABILE_Obj* bab,BB_TCHAR* lpszPho, BB_S16* lpBuffer, BB_U32 nbWanted,BB_U32* samples);

BBT_IMPORT_ BBT_EXPORT_ BABILE_Obj*	 BABILE_init(BB_MemRec* memTab, BABILE_MemParam* babileParams);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_reset(BABILE_Obj* bab);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_free(BABILE_Obj* lpBabil,BB_MemRec* memTab);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_setSetting(BABILE_Obj* lpBabil, BABILE_Param iBabilParam, BB_SPTR iValue);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_getSetting(BABILE_Obj* lpBabil, BABILE_Param iBabilParam, BB_SPTR* piValue);

BBT_IMPORT_ BBT_EXPORT_ BB_ERROR	 BABILE_setDefaultParams(BABILE_Obj* lpBabil);

BBT_IMPORT_ BBT_EXPORT_ void		 BABILE_resetError(BABILE_Obj* lpBabil);

BBT_IMPORT_ BBT_EXPORT_ BB_S16		 BABILE_getLanguage(BABILE_Obj* lpBabil);

#ifdef BABILE_COMPILEFLAG_MALLOCENABLE

BBT_IMPORT_ BBT_EXPORT_ BABILE_Obj*	 BABILE_initEx(BABILE_MemParam* babParam);

BBT_IMPORT_ BBT_EXPORT_  void		 BABILE_freeEx(BABILE_Obj* bab);

#endif


#ifdef __cplusplus
}
#endif
#endif

#if defined( PALMOS5) && !defined(BABILE) /* PALM specific API *//* refNum= Palm library reference number */
BBT_IMPORT_ BB_S16		 BABILE_numAlloc(BB_S16 refNum);
BBT_IMPORT_ BB_S16		 BABILE_alloc(BB_S16 refNum,const BABILE_MemParam* babileParams,BB_MemRec* memTab);
BBT_IMPORT_ BB_TCHAR*	 BABILE_getVersion(BB_S16 refNum);
BBT_IMPORT_ BB_ERROR	 BABILE_changeSpeechFont(BB_S16 refNum,BABILE_Obj* bab, BB_DbLs* SpeechFont);
BBT_IMPORT_ BB_ERROR	 BABILE_getError(BB_S16 refNum,BABILE_Obj* bab, BB_ERROR* mbrError, BB_ERROR* nlpError);
BBT_IMPORT_ BB_S32		 BABILE_readText(BB_S16 refNum,BABILE_Obj* bab,BB_TCHAR* lpszText, BB_S16* lpBuffer, BB_U32 nbWanted,BB_U32* samples);
BBT_IMPORT_ BB_S32		 BABILE_textToPhoStr(BB_S16 refNum,BABILE_Obj* bab,BB_TCHAR* lpszText, BABILE_PhoMode dwPhoMode, BB_TCHAR* lpBuffer, BB_S32 nbWanted,BB_U32 *samples);
BBT_IMPORT_ BB_S32		 BABILE_readPhoStr(BB_S16 refNum,BABILE_Obj* bab,BB_TCHAR* lpszPho, BB_S16* lpBuffer, BB_U32 nbWanted,BB_U32* samples);
BBT_IMPORT_ BABILE_Obj*  BABILE_init(BB_S16 refNum,BB_MemRec* memTab, BABILE_MemParam* babileParams);
BBT_IMPORT_ BB_ERROR	 BABILE_reset(BB_S16 refNum,BABILE_Obj* bab);
BBT_IMPORT_ BB_ERROR	 BABILE_free(BB_S16 refNum,BABILE_Obj* lpBabil,BB_MemRec* memTab);
BBT_IMPORT_ BB_ERROR	 BABILE_setSetting(BB_S16 refNum,BABILE_Obj* lpBabil, BABILE_Param iBabilParam, BB_S32 iValue);
BBT_IMPORT_ BB_ERROR	 BABILE_getSetting(BB_S16 refNum,BABILE_Obj* lpBabil, BABILE_Param iBabilParam, BB_S32* piValue);
BBT_IMPORT_ BB_ERROR	 BABILE_setDefaultParams(BB_S16 refNum,BABILE_Obj* lpBabil);
BBT_IMPORT_ void		 BABILE_resetError(BB_S16 refNum,BABILE_Obj* lpBabil);
BBT_IMPORT_ BB_S16		 BABILE_getLanguage(BB_S16 refNum,BABILE_Obj* lpBabil);
#endif

#endif /* __BB_BABILE__IF_BABILE_H__ */
