#ifndef __BBT__IS_BABILE_CH__
#define __BBT__IS_BABILE_CH__


#ifdef __cplusplus
extern "C"
{
#endif

char const * const BABILE_errorStrings[]={
"NO ERROR",/*					0		*//* no error */
"E_BABILE_NOMEM",/*				-1		*//* no memory for allocation */
"E_BABILE_MEMFREE",/*			-2		*//* problem when deallocating */ 
"E_BABILE_ISPLAYING",/*			-3		*//* already in play mode or currently processing text */
"E_BABILE_SPEAKERROR",/*		-4		*//* Error while Speaking or processing text */
"E_BABILE_WAVEOUTWRITE",/*		-5*/
"E_BABILE_WAVEOUTNOTFREE",/*	-6		*//* Can't open the wavout device */
"E_BABILE_PROCESSERROR",/*		-7*/
"E_BABILE_REGISTRYERROR",/*		-8*/
"E_BABILE_NOREGISTRY",/*		-9		*//* Registry keys error*/
"E_BABILE_NOTVALIDPARAMETER",/*	-10*/
"E_BABILE_THREADERROR",/*		-11*/
"E_BABILE_NONLP",/*				-12*/
"E_BABILE_INVALIDTAG",/*		-13*/
"E_BABILE_FILEWRITE",/*			-14*/
"E_BABILE_FILEOPEN",/*			-15*/
"E_BABILE_BADPHO",/*			-16*/
"E_BABILE_DICT_OPEN",/*			-17*/
"E_BABILE_DICT_WRITE",/*		-18*/
"E_BABILE_DICT_READ",/*			-19	*/
"E_BABILE_DICT_NOENTRY",/*		-20*/
"E_BABILE_NOTIMPLEMENTED",/*	-21*/
"E_BABILE_NODBA",/*				-22		*//* database ERROR */
"E_BABILE_NODICT",/*			-23*/
"E_BABILE_VALUEOUTOFBOUNDS",/*	-24		*//* BABILE */
"E_BABILE_NULLARG",/*			-25*/
"E_BABILE_MBRINITERROR",/*		-26*/
"E_BABILE_NLPINITERROR",/*		-27*/
"E_BABILE_MBRERROR",/*			-28*/
"E_BABILE_PHOSTRPROGRESSERROR",/*-29*/
"E_BABILE_DICT_EXIST",/*		-30*/
"E_BABILE_SELINITERROR",/*		-31*/
"E_BABILE_SELERROR",/*			-32*/
"E_BABILE_NOSEL",/*				-33*/
"E_BABILE_LICENSE",/*			-34	*/
"UNKNOWN ERROR",/*				-35	*/
"UNKNOWN ERROR",/*				-36	*/
"UNKNOWN ERROR",/*				-37	*/
"UNKNOWN ERROR",/*				-38	*/
"UNKNOWN ERROR",/*				-39	*/
"UNKNOWN ERROR",/*				-40	*/
"E_BABILE_SYNTHESIZESF",/*		-41*/
"E_BABILE_PHONBUFERROR",/*		-42*/
"E_BABILE_NOMBR",/*				-43*/
"E_BABILE_NLPERROR",/*			-44*/
"E_BABILE_COLERROR",/*			-45*/
"E_BABILE_NOCOLIBRI",/*			-46*/
"E_BABILE_COLINITERROR",/*		-47*/
"E_BABILE_NOVALIDSYNTH",/*		-48*/
0
};
#define BABILE_MAX_ERROR_VALUE				46


char const * const BABILE_settingStrings[]={
"",	/*				0*/
"",	/*				1*/
"PITCH",/*			2 */
"SPEED",/*			3 */
"",/*				4*/
"VOICEFREQ",/*				5*/
"",/*				6*/
"",/*				7*/
"",/*				8*/
"MAXPITCH",/*		9 */
"MINPITCH",/*		10 */
"VOLUMERATIO",/*	11*/
/*#define BABIL_MINVOL	13
#define BABIL_MAXVOL	899 */
"CBINSTANCE",/*		12*/
"LEADINGSILENCE",/*	13 */
"",/*				14 */
"TRAILINGSILENCE",/*15 */
"",/*				16	*/
"",/*				17*/
"",/*				18*/
"ACTIVEMODULES",/*	19*/
"SENTENCELENGTH",/*	20*/
"PAUSE1SILENCE",/*	21 */
"PAUSE2SILENCE",/*	22 */
"PAUSE3SILENCE",/*	23 */
"PAUSE4SILENCE",/*	24 */
"PAUSE5SILENCE",/*	25*/
"",/*				26*/
"",/*				27*/
"",/*				28*/
"",/*				29*/
"MAXEXTF0",/*		30 */
"MAXINTF0",/*		31 */
"WORDSPELLVOIRATIO",/*32*/
"ACROSPELLCONRATIO",/*33 */
"",/*				34 */
"",/*				35 */
"",/*				36 */
"PTR_DCTCALLOBJ",/*	37*/
"PTR_DCTCALLFCT",/*	38*/
"",/*				39*/
"LANGUAGECODE",/*	40 */
"SPEAKON",/*		41*/
"DCTORDER",/*		42*/
"",/*				43*/
"",/*				44*/
"PTR_TAGCALLFCT",/*	45*/
"PTR_TAGCALLOBJ",/*	46*/
"",/*				47*/
"",/*				48*/
"",/*				49*/
"MBR_FREQ",/*		50*/
"",/*				51*/
"MBR_PITCH",/*		52*/
"MBR_TIME",/*		53*/
"MBR_SMOO",/*		54*/
"MBR_SYNC",/*		55*/
"MBR_VOICEFREQ",/*	56*/
"",/*				57*/
"",/*				58*/
"",/*				59*/
"SEL_NBPRESEL",/*	60*/
"",			/*	61*/
"",/*				62*/
"SEL_SYNC",/*		63*/
"SEL_VOICEFREQ",/*	64*/
"SEL_PITCH",/*		65*/
"SEL_TIME",/*		66*/	
"SEL_VOICECTRL",/*	67*/
"VOICESHAPE",/*	68*/
"SEL_SELBREAK",/*69*/
"",/*70*/
"",/*71*/
"",/*72*/
"",/*73*/
"SEL_SPANPITCH",/*74*/
"PITCHNLP",/*75*/
"SAMPLESIZE",		/*76 */
"PREEMPH",/*			77*/
0
};

char const * const BABILE_sMBRConfigStrings[]={
	"BABILE_MBRE_BUFFERING0",	/*0*/
	"",							/*1*/
	"",							/*2*/
	"",							/*3*/
	"",							/*4*/
	"BABILE_MBRE_BUFFERING5",	/*5*/
	"",							/*6*/
	"",							/*7*/
	"",							/*8*/
	"",							/*9*/
	"BABILE_MBRE_BUFFERING10",	/*10*/
	0
};

char const * const BABILE_sNLPConfigStrings[]={
	"BABILE_NLP_BUFFERING0",	/*0*/
	0
};

char const * const BABILE_sSELConfigStrings[]={
	"AALL"	,/*0*/
	"AMED"	,/*1*/
	"OALL"	,/*2*/
	"OSMA"	,/*3*/
	"BALL"	,/*4*/
	"BSMA"	,/*5*/
	"_F"		,/*6*/
	0
};

#include "ic_selector.h"
	
const BB_U32 BABILE_sSELConfigValues[]={
	BUFFER_UNITACOUSTIC_ALL,	/*"AALL",*//*0*/
	BUFFER_UNITACOUSTIC_MED,	/*"AMED",*//*1*/
	BUFFER_UNITSOFFSETS_ALL,	/*"OALL",*//*2*/
	BUFFER_UNITSOFFSETS_SMALL,	/*"OSMA",*//*3*/
	BUFFER_BERSTREAM_ALL,		/*"BALL",*//*4*/
	BUFFER_BERSTREAM_SMALL,		/*"BSMA",*//*5*/
	BUFFER_FORCE,				/*"F",*//*6*/
	0
};

#ifdef __cplusplus
}
#endif

#endif /*__BBT__IS_BABILE_CH__*/


