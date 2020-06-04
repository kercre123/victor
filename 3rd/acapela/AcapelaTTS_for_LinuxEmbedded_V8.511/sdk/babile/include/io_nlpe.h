#ifndef __BB_NLPE__IO_NLPE_H__
#define __BB_NLPE__IO_NLPE_H__


#ifndef __BB_NLPE__PO_NLPE_H__
typedef void Sdlstts;
typedef void Sf0target;
typedef void Sphoneme;
typedef void NLP_struct;
#endif

typedef struct NLPE_MemParam
{
	BB_U32 line_size; 
} NLPE_MemParam;

/* DICTIONARY CALLBACK */
typedef BB_S32 (*NLPE_DctCallBack) (void* pvUsr, const BB_TCHAR* word, BB_S32 s32Index, BB_TCHAR* trans, BB_S16* ps16TransSize,BB_U8* nat,BB_S16* ps16NatSize,BB_U8* pu8NbTrans, BB_S16 pos); 

/* 
return BB_S32:	->Positive or null value indicates that the word is found.
				  The value gives the index of the word in the dictionary.
				->Negative value indicates that the word is not found (-1) 
				  or an error (other negative values).
void* pvUsr:	User dictionary object.
BB_TCHAR* word:		word to transcribe.
BB_S32 s32Index: s32Index is the index of the word to be retrieved. It overrides word field.
				If the Index is unknown s32Index should be -1.
BB_TCHAR* trans:	buffer that will contain the phonetic transcription.
				This pointer can be NULL if the transcription is not needed
				(If the function is used to retrieve the index or the possible gramatical natures).
BB_S16* ps16TransSize:->If trans is NULL: ps16TransSize will be filled with the length of the phonetic
						transcription of word or s32Index.
					  ->If trans is not NULL: *ps16TransSize is the size of the trans buffer (in bytes).
						It will be overwritten with the length of the phonetic transcription of word or s32Index.
						It can be therefore use to check if the transcription was truncated.
					  If *ps16TransSize is < to the needed size, the phonetic string is truncated and zero terminated.
BB_U8* nat:		Buffer that will contain the possible gramatical natures and their probabilities of occurance.
				Each nature (number ranging from 1 to 255) and probability (from 1 to 100, in %)
				is a byte. 
				The couples (nature/probability) are stored together (nat1, prob1, nat2, prob2, nat3, prob3).
				This pointer can be NULL if not needed.
BB_S16* ps16NatSize:	->If nat is NULL: ps16NatSize will be filled with the size of the needed size of nat buffer.
						  It indicates the number of natures availlables for the word.
						  There are actually couples (nature,probability).
						  The returned *ps16NatSize is 1 or 2 if there is just one transcription (so one nature, with
						  probability 100%).
						  The number of availlables natures is computed from the returned *ps16NatSize by:
						  = (*ps16NatSize+1)/2
						->If nat is not null: *ps16NatSize is the size of the nat buffer (in bytes).
						  It will be overwritten with the actual needed size.
						  It can be therefore be used to check if the vector was truncated.
						If *ps16NatSize is < to the needed size, the string is truncated but not zero terminated, 
						So that, if s16SizeOrIndex=1 then just the first nature is retrieved in a one byte storage space.
BB_U8* pu8NbTrans:      Will contain the number of availables alternatives transcriptions.
                        The actual number of transcriptions is then pu8NbTrans+1
BB_S16 pos:		It is the Index of the wanted transcription (if	several transcriptions exist (default is 0)).


*/

/*NLPE TAG CALLBACK */

typedef struct {
	BB_S32 in_size;			/* size of the structure itself */
	BB_S32 out_size;		/* size of the structure itself (for confirmation)*/
	BB_TCHAR* in_name;		/* tag name */
	BB_TCHAR* in_val;		/* tag value */
	BB_U32 in_position;		/* tag position in the current buffer */
	BB_TCHAR* out_newname;	/* new tag name */
	BB_TCHAR* out_newval;	/* new tag value */
} TagStr;

typedef BB_U32 (*NLPE_TagCallBack) (void* obj,TagStr* tagstr); 

/*typedef BB_S32 (*NLPE_MrkCallBack) (BB_S32 callback_type, BB_S32 pos_txt, BB_S32 pos_sampl, void* call_env);*/



#endif /* __BB_NLPE__IO_NLPE_H__ */
