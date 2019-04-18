#ifndef __BABILE__IO_BABILE_H__
#define __BABILE__IO_BABILE_H__

#ifndef BABILE_EVENTCALLBACKSTRUCT
typedef struct BABILE_sEventCallBackStruct /* [OUT, given by TTS] !! scratch memory */
{
	BB_U32 size;			/* sizeof the structure itself (should be >= to BABILE_sEventCallBackStruct) */
	BB_S32 event_type;		/* CallBack type (?_MARK_TYPE_WORD, ?_MARK_TYPE_SENTENCE...)*/
	void* object;			/* pointer to an user defined object */
	BB_S32 pos;				/* (start) position of the event (in chars) in the current text buffer */
	BB_S32 off;				/* offset of the Event (in Samples) in the current audio buffer*/
	BB_S32 len;				/* len in the text (in chars) */
	BB_S32 viseme;			/* associated viseme ( only for phonemes/visemes callback) */
} BABILE_EventCallBackStruct;
#endif 

typedef BB_S32 ( *BB_pMarkCallback)   (BABILE_EventCallBackStruct* /*[OUT, scratch]*/);

typedef struct BABILE_sMemParam
{
	BB_S32 sSize;
	BB_S32 sMBRConfig; 
	BB_S32 sSELConfig; 
	BB_S32 sNLPConfig; 
	BB_S32 nlpModule;
	BB_S32 synthModule;
	BB_DbLs* synthLS;	
	BB_DbLs* nlpeLS;	
	void* u32MarkCallbackInstance;  /* Allows to store an user defined object pointer inside BABILE object, the pointer is then returned by the callback */
	BB_pMarkCallback markCallback;
	BB_ERROR nlpInitError;
	BB_ERROR mbrInitError;
	BB_ERROR selInitError;
	BB_ERROR colInitError;
	BB_ERROR initError;
	const BB_TCHAR* license; 
	struct {
		BB_U32 userId;
		BB_U32 passwd;
	} uid;
} BABILE_MemParam; 

typedef BB_S32 BABILE_PhoMode;
typedef BB_S32 BABILE_Param;

#ifndef __BABILE__PO_BABILE_H__

typedef void BABILE_Obj;

#endif /* __BABILE__PO_BABILE_H__ */

#endif /* __BABILE__IO_BABILE_H__ */
