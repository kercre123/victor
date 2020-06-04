#ifndef __BBT__ISTAG_BABILE_H__
#define __BBT__ISTAG_BABILE_H__

#include "io_nlpe.h"  /* to include NLPE TagStr definition */

/* THIS FILE CONTAINS EXAMPLES OF IMPLEMENTATIONS OF TAG CALLBACKS */


/* Example 1:	allows to call setSettings when the text is parsed by the NLP engine.
				Caution, this callBack is called when the text is parsed, so the setting is applied at this
				time. */
char const * const BABILE_tagStrings[]={
"rst",	/*					0	--> RESET = set default values */
"", /*						1 */
"pit",/*PITCH",				2 */
"spd", /*SPEED",			3 */
"",/*						4 */
"",/*						5 */
"",/*						6 */
"",/*						7 */
"",/*						8 */
"map",/*MAXPITCH",			9 */
"mip",/*MINPITCH",			10 */
"vol",/*VOLUMERATIO",		11 */
"",/*CBINSTANCE",			12 */
"lsi",/*LEADINGSILENCE",	13 */
"",/*						14 */
"tsi",/*TRAILINGSILENCE",	15 */
"",/*						16 */
"",/*						17 */
"",/*						18 */
"sam",/*		ACTIVEMODULES",	19 */
"slh",/*SENTENCELENGTH",	20 */
"p1s",/*PAUSE1SILENCE",		21 */
"p2s",/*PAUSE2SILENCE",		22 */
"p3s",/*PAUSE3SILENCE",		23 */
"p4s",/*PAUSE4SILENCE",		24 */
"p5s",/*PAUSE5SILENCE",		25 */
"",/*						26 */
"",/*						27 */
"",/*						28 */
"",/*						29 */
"ef0",/*MAXEXTF0",			30 */
"if0",/*MAXINTF0",			31 */
"wsv",/*WORDSPELLVOIRATIO",	32 */
"asc",/*ACROSPELLCONRATIO",	33 */
"",/*						34 */
"",/*						35 */
"",/*						36 */
"dco",/*PTR_DCTCALLOBJ",	37 */
"dcf",/*PTR_DCTCALLFCT",	38 */
"",/*						39 */
"",/*LANGUAGECODE",			40 */
"",/*SPEAKON",				41  --> use rms,rmw*/
"dor",/*DCTORDER",			42*/
"",/*						43 */
"",/*						44 */
"tcf",/*PTR_TAGCALLFCT"		45 */
"tco",/*PTR_TAGCALLOBJ"		46 */
"",/*						47 */
"",/*						48 */
"",/*						49 */
"mbf",/*BABIL_PARM_MBR_FREQ 50 */
"",/*						51 */
"mbp",/*BABIL_PARM_MBR_PITCH52 */
"mbt",/*BABIL_PARM_MBR_TIME	53 */
"mbo",/*BABIL_PARM_MBR_SMOO	54 */
"mby",/*BABIL_PARM_MBR_SYNC	55 */
"mbv",/*BABIL_PARM_MBR_VOICEFREQ 56 *//* Voice Frequency */
"",/*						57 */
"",/*						58 */
"",/*						59 */
"sen",/*BABIL_PARM_SEL_NBPRESEL	60*/
"",/*						61 */
"",/*						62 */
"sey",/*BABIL_PARM_SEL_SYNC	63 */
"sev",/*BABIL_PARM_SEL_VOICEFREQ 64*/
"sep",/*BABIL_PARM_SEL_PITCH65*/
"set",/*BABIL_PARM_SEL_TIME	66	*/
"sec",/*BABIL_PARM_SEL_VOICECTRL (HNM) 67 */
"ses",/*BABIL_PARM_SEL_VOICESHAPE 68 */
"seb",/*BABIL_PARM_SEL_SELBREAK 69 */
"",/*BABIL_PARM_SEL_SELBREAK 70 */
"",/*BABIL_PARM_SEL_SELBREAK 71 */
"",/*BABIL_PARM_SEL_SELBREAK 72 */
"",/*BABIL_PARM_SEL_SELBREAK 73 */
"sap",/*BABIL_PARM_SEL_SPANAROUNDVOICEPITCHDELTA 74 */
"pnl",/* PITCHNLP 75*/
"",		/*SAMPLESIZE 76 */
"audioboost",/* BOOST	PREEMPH		77*/
NULL
};

/*REAMARK "rmw", "rms", "pau", "prn" are build in */

static BB_U32 BABILE_NLPE_tagCallFct(BABILE_Obj* obj,TagStr* tagstr)
{/* obj has to be BABILE_Obj */
	BB_U32 store=1;
	BB_TCHAR* name;
	BB_U8 i=0;
	if (!tagstr || !tagstr->in_name || !tagstr->in_val || !obj  || tagstr->in_size<sizeof(TagStr)) return store;
	tagstr->out_size=sizeof(TagStr);
	name=tagstr->in_name;
	if (*name=='#') {store=0;name++;}
	while (BABILE_tagStrings[i] && strcmp((const char*)name,BABILE_tagStrings[i])) i++;
	if (BABILE_tagStrings[i]) 
	{
		if (i>=BABIL_PARM_PITCH)
		{
			BABILE_setSetting((BABILE_Obj*) obj,i,atoi((const char*)tagstr->in_val));
			BABILE_resetError((BABILE_Obj*) obj);
		}
		else if (i==0) /* RST */
		{
			BABILE_setDefaultParams((BABILE_Obj*) obj);
		}
	}
	return store;
}

#ifdef _ISTAG_BABILE_USE_ALIAS_
/* Example 2:	Callback used to handle tag Aliases
*/

#include "dic203cb.h" /* it uses user lexicon callback functions to perform the tag lookup */

static BB_U32 ALIAS_tagCallFct(void* obj,TagStr* tagstr)
{ /* obj has to be a SimpleDictionary */
	BB_U32 store=1;
	BB_TCHAR* tag, *ist;
	static BB_TCHAR newName[12];
	int len=0;
	if (!tagstr || !tagstr->in_name || !tagstr->in_val || !obj || tagstr->in_size<sizeof(TagStr)) return store;
	tagstr->out_size=sizeof(TagStr);
	if (!strcmp((const char*)tagstr->in_name,"alias"))
	{
		/* finds the tag value in a tag dictionary */
		if (dicSimpleCb((SimpleDictionary*)obj,(char*)tagstr->in_val,(char**)&tag,NULL,-1)>=0)
		{/* replaces and store the sel=Unit tag */
			if ((ist=(BB_TCHAR*)strchr((const char*)tag,'='))!=NULL)
			{
				tagstr->out_newval=&ist[1];
				tagstr->out_newname=newName;
				len=((BB_UPTR)ist-(BB_UPTR)tag)>sizeof(newName)?(int)sizeof(newName):(int)((BB_UPTR)ist-(BB_UPTR)tag);
				strncpy(( char*)newName,(const char *)tag,len);
				newName[len]=0;
				store=2;
			}
			else
			{
				store=0;
			}
		}
		else
		{/* else remove the tag */
			store=0;
		}
	}
	return store;
}
static BB_U32 EXPAND_tagCallFct(void* obj,TagStr* tagstr)
{ /* obj has to be a SimpleDictionary */
	BB_U32 store=1;
	BB_TCHAR* tag;
	static BB_TCHAR newName[12];
	if (!tagstr || !tagstr->in_name || !tagstr->in_val || !obj || tagstr->in_size<sizeof(TagStr)) return store;
	tagstr->out_size=sizeof(TagStr);
	if (!strcmp((const char*)tagstr->in_name,"expand"))
	{
		/* finds the tag value in a tag dictionary */
		if (dicSimpleCb((SimpleDictionary*)obj,( char*)tagstr->in_val,(char**)&tag,NULL,-1)>=0)
		{/* replaces and store the sel=Unit tag */
				tagstr->out_newval=NULL;
				tagstr->out_newname=tag;
				store=2;
		}
		else
		{/* else remove the tag */
			store=0;
		}
	}
	return store;
}

#endif /* _ISTAG_BABILE_USE_ALLIAS_*/
#ifdef _ISTAG_BABILE_USE_MY_
typedef struct {
	void* synthetizer;
	void* dict;
	void* txtQueue;
} my_tagCallBackObj ;


static BB_U32 MY_tagCallFct(void* obj,TagStr* tagstr)
{
	my_tagCallBackObj* o=(my_tagCallBackObj*) obj;
	if (tagstr && tagstr->in_size>=sizeof(TagStr) && obj)
	{
#ifdef _ISTAG_BABILE_USE_ALIAS_

		if (!strcmp((const char*)tagstr->in_name,"alias") && o->dict)
		{
			return ALIAS_tagCallFct(o->dict,tagstr);
		}
		else 
		if (!strcmp((const char*)tagstr->in_name,"expand") && o->dict)
		{
			return EXPAND_tagCallFct(o->dict,tagstr);
		}
		else
#endif /* _ISTAG_BABILE_USE_ALLIAS_ */
#ifdef __ACA_BB_TAGHNDL_H__
		if (!_stricmp((const char*)tagstr->in_name,"vct") || !_stricmp((const char*)tagstr->in_name,"vol") || !_stricmp((const char*)tagstr->in_name,"rst"))
		{/* delays tag handling to synchronise it with output samples */
			return TAGQUEUE_tagCallFct((TxtItem*)(o->txtQueue),tagstr);
		}
		else
#endif /* __ACA_BB_TAGHNDL_H__ */
		{
			return BABILE_NLPE_tagCallFct(o->synthetizer,tagstr);
		}
	}
	return 1;
}
#endif /* _ISTAG_BABILE_USE_MY_ */


#endif /*__BBT__IS_BABILE_H__*/


