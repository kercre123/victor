/*
 nscube.h
 NETworks Cube / Speech Cube API header file
 Copyright Elan Speech 2003
*/

#ifndef NSCUBE_H
#define NSCUBE_H

/* Types definitions */

#ifdef _WIN32
#ifdef NSCUBE_EXTERN
	#ifdef __cplusplus
		#define NSCUBE_API extern "C"
	#else
		#define NSCUBE_API extern
	#endif
#elif defined NSCUBE_EXPORTS
	#ifdef __cplusplus
		#define NSCUBE_API extern "C" __declspec(dllexport)
	#else
		#define NSCUBE_API __declspec(dllexport)
	#endif
#else
	#ifdef __cplusplus
		#define NSCUBE_API extern "C" __declspec(dllimport)
	#else
		#define NSCUBE_API extern __declspec(dllimport)
	#endif
#endif
#else
/* Non Windows shared library export */
	#ifdef __cplusplus
		#define NSCUBE_API extern "C" __attribute__ ((visibility ("default"))) 
	#else
		#define NSCUBE_API extern __attribute__ ((visibility ("default"))) 
	#endif

	#define WINAPI
#endif /* _WIN32 */


typedef unsigned long	nscCHANID;
typedef void *			nscHANDLE;
typedef void *			nscHSRV;

#define NSC_MAX_VOICE_LEN 255

/* Language (windows-like) identifiers */


#define FRENCH				1036	/* french					*/
#define BRAZILIAN			1046	/* brazilian				*/
#define GERMAN				1031	/* german					*/
#define ENGLISH				2057	/* english					*/
#define BRITISH				2057	/* english					*/
#define ITALIAN				1040	/* italian					*/
#define SPANISH				1034	/* spanish					*/
#define AMERICAN			1033	/* us english				*/
#define USENGLISH			1033	/* us english				*/
#define RUSSIAN				1049	/* russian					*/
#define POLISH				1045	/* polish					*/
#define CZECH				1029	/* czech					*/
#define DUTCH				1043	/* dutch					*/
#define ARABIC				5121    /* arabic (ALGERIA)         */
#define ARABIC_SA			1025    /* arabic (SAUDI ARABIA)    */
#define TURKISH				1055	/* turkish					*/
#define DANISH				1030	/* danish					*/
#define PORTUGUESE			2070	/* portuguese				*/
#define ICELANDIC			1039	/* icelandic				*/
#define FINNISH				1035	/* finnish					*/
#define SWEDISH				1053	/* swedish					*/
#define NORWEGIAN			1044	/* norwegian				*/
#define BELGIANDUTCH		2067	/* belgian dutch			*/
#define BELGIANFRENCH		2060	/* belgian french			*/
#define JAPANESE			1041	/* japanese 				*/
#define KOREAN				1042    /* korean 					*/
#define CHINESE				2052	/* mandarin chinese		    */
#define MANDARINCHINESE		2052	/* mandarin chinese		    */
#define CANADIANFRENCH		3084	/* canadian french			*/
#define FAROESE				1080	/* faroese					*/
#define GREEK				1032	/* greek					*/
#define INDIANENGLISH		16393	/* indian english			*/
#define FINNISHSWEDISH		2077	/* finland swedish		    */
#define FINLANDSWEDISH		2077	/* finland swedish		    */
#define CATALAN				1027	/* catalan					*/
#define USSPANISH           21514   /* us spanish 				*/
#define SCANIAN				62493	/* scanian swedish		    */
#define GOTHENBURGSWEDISH   58397   /* gothenburg swedish       */
#define AUSTRALIAN          3081    /* australian english       */
#define AUSTRIANGERMAN      3079    /* austrian german          */
#define SAUDIARABIANENGLISH 49161   /* saudi arabia english     */
#define SCOTTISHENGLISH     1537    /* scottish english         */

/*
Return value for nscRESULT
	Error are values < 0
	Warning are values > 0
*/

#define	NSC_OK						0

/* Error codes */

#define	NSC_NOT_ENOUGH_MEMORY		-1
#define NSC_ERR_INVALID_HANDLE		-2
#define NSC_ERR_INVALID_CHANNEL		-3
#define NSC_ERR_INVALID_SERVER		-4
#define NSC_ERR_VOICE_LIST			-5
#define NSC_ERR_IDVOICE				-6
#define NSC_ERR_VOICE_INIT			-7
#define NSC_ERR_PROCESS_DATA		-8
#define NSC_ERR_EXEC_STARTED		-9
#define NSC_ERR_INVALID_PARAM		-10
#define NSC_ERR_RANGE_PARAM			-11
#define NSC_ERR_CHANNEL_LOCKED		-12
#define NSC_ERR_CHANNEL_NOTREADY	-13
#define NSC_ERR_CONNECT				-14
#define NSC_ERR_TIMEOUT_INIT		-15
#define NSC_ERR_CONNRESET			-16
#define NSC_ERR_CONNREJECT			-17
#define NSC_ERR_TOOMANY_VOICES		-18
#define NSC_ERR_AUDIO_INIT			-19
#define NSC_ERR_INVALID_FORMAT		-20
#define NSC_ERR_INVALID_FREQ		-21
#define NSC_ERR_ACTIVE_CHANNEL		-22
#define NSC_ERR_CHANNEL_DOWN		-23
#define NSC_ERR_INVALID_POINTER		-24
#define NSC_INVALID_PARAM			-25
#define NSC_INVALID_LICENSE			-26
#define NSC_NOT_IMPLEMENTED_YET		-50

/* Warning codes */

#define NSC_NO_MATCHING_VOICE	1
#define NSC_ALREADY_STARTED		2
#define NSC_CHANNEL_NOTSTARTED	3
#define NSC_TIMEOUT_EVENT		4
#define NSC_CHANNEL_LOCKED		5
#define NSC_CHANNEL_UNLOCKED	6
#define NSC_AUTO_DISPATCH		7
#define NSC_CLIENT_CONNECTED	8
#define NSC_CHANNEL_STOPPED		9
#define NSC_CHANNEL_NOTEXT		10
#define NSC_CHANNEL_PAUSED		11
#define NSC_SRV_NOTRUNNING		12
#define NSC_SRV_NOTOEM_PROT		13
#define NSC_CHANNEL_TIMEOUT		14

typedef int	nscRESULT;

/* Address family type enum */

typedef enum _NSC_AFTYPE_ENUM
{
	NSC_AF_DEFAULT=0,
	NSC_AF_LOCAL,
	NSC_AF_INET,
    NSC_AF_DIRECT
} NSC_AFTYPE_ENUM;


/* Server type enumeration */

typedef enum _NSC_SRVTYPE_ENUM
{
	NSC_STYPE_STANDALONE=0,
	NSC_STYPE_MASTER,
	NSC_STYPE_SLAVE,
	NSC_STYPE_MASTER_SLAVE,
    NSC_STYPE_INPROCESS
} NSC_SRVTYPE_ENUM;

/* Server status type enumeration */
typedef enum _NSC_SRVSTATUS_ENUM
{
	NSC_SSTAT_OFF=0,
	NSC_SSTAT_RUNNING,
	NSC_SSTAT_DOWN
} NSC_SRVSTATUS_ENUM;

/* Server AuthRateCtrl type enumeration */
typedef enum _NSC_AUTHCTRL_ENUM
{
    NSC_STDRTP=0,
    NSC_ULRTP,
    NSC_BADRTP,
    NSC_HNMRTP,
    NSC_TCMRTP
} NSC_AUTHCTRL_ENUM;

/* Parameter type enumeration */

typedef enum _NSC_PRMTYPE_ENUM
{
	NSC_PRM_PITCH=1,
	NSC_PRM_SPEED,
	NSC_PRM_VOL,
	NSC_PRM_EVTMASK,
	NSC_PRM_SHAPE,
	NSC_PRM_SELBREAK,
	NSC_PRM_PRESEL,
	NSC_PRM_AUDIOBOOST,
	NSC_PRM_LAST
} NSC_PRMTYPE_ENUM;

#define NSCAPI_SAPI_TAG 99
/* Event notification requested by the application */
#define NSC_EVTBIT_TEXT         1
#define NSC_EVTBIT_WORD_SYNCH   2
#define NSC_EVTBIT_PHO_SYNCH    4
#define NSC_EVTBIT_BOOKMARK     8
#define NSC_EVTBIT_TTSERROR     16
#define NSC_EVTBIT_STOP_DONE    32
#define NSC_EVTBIT_MOUTH_POS	64
#define NSC_EVTBIT_WORD_LEN		128
#define NSC_EVTBIT_VISEME		256

/* Event Identifier enumeration */
typedef enum _NSC_EVID_ENUM
{
	NSC_EVID_TEXT_STARTED,
	NSC_EVID_TEXT_DONE,
	NSC_EVID_WORD_SYNCH,
	NSC_EVID_PHO_SYNCH,
	NSC_EVID_BOOKMARK,
	NSC_EVID_TTSERROR,
    NSC_EVID_STOP_DONE,
	NSC_EVID_MOUTH_POS,
	NSC_EVID_WORD_LEN,
	NSC_EVID_VISEME,
	NSC_EVID_LAST
} NSC_EVID_ENUM;

/* Voice encoding */
#define NSC_VOICE_ENCODING_PCM		1 /* 16bits pcm mono */
#define NSC_VOICE_ENCODING_A_LAW	6 /* 8bits alaw mono */
#define NSC_VOICE_ENCODING_MU_LAW	7 /* 8bits mulaw mono */

/* Voice description structure */
typedef struct _NSC_FINDVOICE_DATA
{
	int nGender;
	int	nLanguage;
	int	nInitialSampleFreq;
	int	nInitialCoding;
	int nOutputSampleFreq;
	int nOutputCoding;
	char cSpeakerName[NSC_MAX_VOICE_LEN];
	char cDisplayName[NSC_MAX_VOICE_LEN];
	char cVoiceName[NSC_MAX_VOICE_LEN];
	char cISOCode[NSC_MAX_VOICE_LEN];
	char cVoiceQuality[NSC_MAX_VOICE_LEN];
	char cVoiceLanguage[NSC_MAX_VOICE_LEN];
	int cCodePage;
} NSC_FINDVOICE_DATA, *PNSC_FINDVOICE_DATA;


/* Server info structure */

typedef struct _NSC_SRVINFO_DATA
{
	int nServerType;
	int nServerStatus;
	int nServerVersion;
	int nMaxNbVoice;
	int nAuthRateCtrl;
	int nAutMaxNbChannel;
	int nAuthMaxRTRate;
	int nCurRTRate;
	int nCurNbChannel;
}  NSC_SRVINFO_DATA, *PNSC_SRVINFO_DATA;

#define NSC_STDRTP		0
#define NSC_ULTRP		1
#define NSC_TCMRTP		4

/* Generic event data structure */
typedef struct _NSC_EVENT_DATA
{
    unsigned int	uiSize;
    unsigned char	bData[1];
} NSC_EVENT_DATA, *PNSC_EVENT_DATA;

/* Specific event data structures */
typedef struct
{
    unsigned int	uiSize;
    void			*pUserData ;
} NSC_EVENT_DATA_TextStarted ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData ;
} NSC_EVENT_DATA_TextDone ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    unsigned int	uiWordPos;				/* position in bytes of first char in text  */
	unsigned int	uiByteCount;			/* position in bytes in whole audio signal	*/
} NSC_EVENT_DATA_WordSynch ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    unsigned int	uiWordPos;				/* position in bytes of first char in text  */
	unsigned int	uiWordLen;				/* length in bytes of the word */
	unsigned int	uiByteCount;			/* position in bytes in whole audio signal	*/
} NSC_EVENT_DATA_WordLen ;

#define MAXCHAR_PHONEME 2
#define MAX_IPA_PAR_PHO 2

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    char            cEngine_Phoneme[MAXCHAR_PHONEME];	/* ELAN Speech phoneme						*/
    unsigned short  uiIpa_Phoneme[MAX_IPA_PAR_PHO];		/* IPA phoneme								*/
    unsigned int    uiDuration ;						/* phoneme duration in ms					*/
	unsigned int	uiByteCount;						/* position in bytes in whole audio signal	*/
} NSC_EVENT_DATA_PhoSynch ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    char            cEngine_Phoneme[MAXCHAR_PHONEME*2];	/* Acapela phoneme						*/
    unsigned short  uiIpa_Phoneme;						/* IPA phoneme								*/
    unsigned int    uiDuration ;						/* phoneme duration in ms					*/
	unsigned int	uiByteCount;						/* position in bytes in whole audio signal	*/
} NSC_EVENT_DATA_AcaPhoSynch ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    unsigned int    uiDuration ;						/* phoneme duration in ms					*/
	unsigned int	uiByteCount;						/* position in bytes in whole audio signal	*/
	unsigned int	uiVisemeId;							/* current viseme ID						*/
	unsigned int	uiNextVisemeId;						/* next viseme ID							*/
} NSC_EVENT_DATA_Viseme ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    char            cEngine_Phoneme[MAXCHAR_PHONEME*2];	/* Acapela phoneme						*/
    unsigned short  uiIpa_Phoneme;						/* IPA phoneme								*/
    unsigned int    uiDuration ;						/* phoneme duration in ms					*/
	unsigned int	uiByteCount;						/* position in bytes in whole audio signal	*/
	unsigned char	uiMouthPos[8];
} NSC_EVENT_DATA_MouthPos ;

typedef struct
{
    unsigned int	uiSize;
    void			*pUserData;
    unsigned int	uiVal;								/* bookmark value */
	unsigned int	uiByteCount;						/* position in bytes in whole audio signal	*/
} NSC_EVENT_DATA_Bookmark ;


/* Sound data structure */
typedef struct _NSC_SOUND_DATA
{
	unsigned int	uiSize;
	unsigned char	*pSoundBuffer;
} NSC_SOUND_DATA, *PNSC_SOUND_DATA;


/* TTS server protection types */
typedef enum _NSC_SRVPROT_ENUM
{
	NSC_SPROT_DONGLE = 1,
	NSC_SPROT_OEM,
	NSC_SPROT_EVAL,
	NSC_SPROT_EXPIRED,
	NSC_SPROT_TIME
} NSC_SRVPROT_ENUM;


/* Prototype of the call back function used to retreive the sound data from the TTS */
typedef int PNSC_FNSPEECH_DATA (const unsigned char *pData,
								unsigned int cbDataSize,
								PNSC_SOUND_DATA pSoundData,
								void *pAppInstanceData);

/* Prototype of the call back function used to retreive the events from the TTS */
typedef int PNSC_FNSPEECH_EVENT(unsigned int nEventID,
								unsigned int cbEventDataSize,
								PNSC_EVENT_DATA pEventData,
								void *pAppInstanceData);


typedef struct _NSC_EXEC_DATA
{
	PNSC_FNSPEECH_DATA *pfnSpeechData;
	PNSC_FNSPEECH_EVENT *pfnSpeechEvent;
	unsigned long ulEventFilter;
	int bEventSynchroReq;
	NSC_SOUND_DATA vsSoundData;
	void *pAppInstanceData;
} NSC_EXEC_DATA, *PNSC_EXEC_DATA;

/* Function prototypes */

NSCUBE_API nscRESULT WINAPI nscCreateServerContext
(
	int nAddressFamily,
	int nPort,
	const char * pSrvAddress,
	nscHSRV *phSrv
);

NSCUBE_API nscRESULT WINAPI nscCreateServerContextEx
(
	int nAddressFamily,
	int nCmdPort,
	int nDataPort,
	const char * pSrvAddress,
	nscHSRV *phSrv
);

NSCUBE_API nscRESULT WINAPI nscReleaseServerContext
(
	nscHSRV hSrv
);

NSCUBE_API nscRESULT WINAPI nscGetServerType
(
	nscHSRV hSrv,
	NSC_SRVTYPE_ENUM *pServerType
);

NSCUBE_API nscRESULT WINAPI nscGetServerInfo
(
	nscHSRV hSrv,
	PNSC_SRVINFO_DATA pSrvInfoData
);

NSCUBE_API nscRESULT WINAPI nscFindFirstVoice
(
	nscHSRV hSrv,
	const char *pVoiceName,
	int nSampleFreq,
	int nLanguage,
	int nGender,
	PNSC_FINDVOICE_DATA pFindVoiceData,
	nscHANDLE *phVoice
);

NSCUBE_API nscRESULT WINAPI nscFindNextVoice
(
	nscHANDLE hFindVoice,
	PNSC_FINDVOICE_DATA pFindVoiceData
);

NSCUBE_API nscRESULT WINAPI nscCloseFindVoice
(
	nscHANDLE hFindVoice
);

NSCUBE_API nscRESULT WINAPI nscCreateDispatcher
(
	nscHANDLE *phDispatch
);

NSCUBE_API nscRESULT WINAPI nscDeleteDispatcher
(
	nscHANDLE phDispatch
);

NSCUBE_API nscRESULT WINAPI nscInitChannel
(
	nscHSRV hSrv,
	const char *pVoiceList,
	int nSampleFreq,
	int nCoding,
	nscHANDLE pDispatch,
	nscCHANID *pChId,
	const char * voiceMode = ""
);

NSCUBE_API nscRESULT WINAPI nscCloseChannel
(
	nscHSRV hSrv,
	nscCHANID ChId
);

NSCUBE_API nscRESULT WINAPI nscLockChannel
(
	nscHSRV hSrv,
	nscCHANID ChId,
	nscHANDLE pDispatch,
	nscHANDLE *phTTS
);

NSCUBE_API nscRESULT WINAPI nscUnlockChannel
(
	nscHANDLE hTTS
);

NSCUBE_API nscRESULT WINAPI nscNbVoice
(
	nscHANDLE hTTS,
	int *pNbVoice
);

NSCUBE_API nscRESULT WINAPI nscInfoVoice
(
	nscHANDLE hTTS,
	int nVoice,
	PNSC_FINDVOICE_DATA pFindVoice
);

NSCUBE_API nscRESULT WINAPI nscSwitchVoice
(
	nscHANDLE hTTS,
	int nVoice
);

NSCUBE_API nscRESULT WINAPI nscSwitchVoiceEx
(
	nscHANDLE hTTS,
	const char *pszVoice
);

NSCUBE_API nscRESULT WINAPI nscAddText
(
	nscHANDLE hTTS,
	const char *pszInputTxt,
	void *pUserText
);

NSCUBE_API nscRESULT WINAPI nscAddTextW
(
	nscHANDLE hTTS,
	const wchar_t *pwszInputTxt,
	void *pUserText
);

NSCUBE_API nscRESULT WINAPI nscAddTextEx
(
	nscHANDLE hTTS,
        const char *pszInputEncoding,
        const void *pszInputText,
        const size_t sInputTextSize,
	void *pUserText
);

NSCUBE_API nscRESULT WINAPI nscExecChannel
(
	nscHANDLE hTTS,
	PNSC_EXEC_DATA pExecData
);

NSCUBE_API nscRESULT WINAPI nscStartChannel
(
	nscHANDLE hTTS,
	PNSC_EXEC_DATA pExecData
);

NSCUBE_API nscRESULT WINAPI nscGetEvent
(
	nscHANDLE hDispatch,
	int nTimeOut
);

NSCUBE_API nscRESULT WINAPI nscProcessEvent
(
	nscHANDLE hDispatch
);

NSCUBE_API nscRESULT WINAPI nscGetandProcess
(
	nscHANDLE hDispatch,
	int nTimeOut
);

NSCUBE_API nscRESULT WINAPI nscExitChannel
(
	nscHANDLE hTTS
);

NSCUBE_API nscRESULT WINAPI nscPauseChannel
(
	nscHANDLE hTTS
);

NSCUBE_API nscRESULT WINAPI nscGetParamChannel
(
	nscHANDLE hTTS,
	NSC_PRMTYPE_ENUM nParam,
	int *pnParamValue

);

NSCUBE_API nscRESULT WINAPI nscSetParamChannel
(
	nscHANDLE hTTS,
	NSC_PRMTYPE_ENUM nParam,
	int nParamValue
);

NSCUBE_API nscRESULT WINAPI nscGetParamChannelEx
(
	nscHANDLE hTTS,
	int nType,
	int nParam,
	int *pnParamValue

);

NSCUBE_API nscRESULT WINAPI nscSetParamChannelEx
(
	nscHANDLE hTTS,
	int nType,
	int nParam,
	int nParamValue
);

NSCUBE_API nscRESULT WINAPI nscGetServerProt
(
	nscHSRV hSrv,
	NSC_SRVPROT_ENUM *pServerProt
);

NSCUBE_API nscRESULT WINAPI nscSetOemKEY
(
    nscHSRV hSrv,
    char *pszKey
);

NSCUBE_API nscRESULT WINAPI nscServerShutdown
(
	nscHSRV hSrv,
	int nState
);

NSCUBE_API nscRESULT WINAPI nscSetVoicesPaths
(
	nscHSRV hSrv,
	char *paths
);

NSCUBE_API nscRESULT WINAPI nscSetLog
(
	nscHSRV hSrv,
	char *logPath, 
	int logLevel
);

NSCUBE_API char * WINAPI nscGetVersion
(
	
);


#endif /* NSCUBE_H */
