#ifndef __ACA__BB__IC_SELECTOR_H__
#define __ACA__BB__IC_SELECTOR_H__


static const int MAX_PHO= 200;  /* Max nb_targets in a _ to _ chunk */
#ifdef EMBEDDED /* should be lower for embedded */
#define BBSELECTOR_DEFAULT_MAX_NB_PRESEL 90
#else
#define BBSELECTOR_DEFAULT_MAX_NB_PRESEL 90
#endif

#ifdef __LZ_BABTTS__
static const int MAX_SAMPLE= 2*65536;
#else
static const int MAX_SAMPLE= 0x10000;
#endif /*  __LZ_BABTTS__ */

#ifdef __cplusplus
/*const uint32 BUFFER_UNITACOUSTIC_SMALL=	001000000000; Not available*/
static const unsigned long BUFFER_UNITACOUSTIC_MED=	002000000000;
static const unsigned long BUFFER_UNITACOUSTIC_ALL=	004000000000;
static const unsigned long BUFFER_UNITSOFFSETS_SMALL= 000100000000;
static const unsigned long BUFFER_UNITSOFFSETS_ALL=	000400000000;
static const unsigned long BUFFER_BERSTREAM_SMALL=	000010000000;
static const unsigned long BUFFER_BERSTREAM_ALL=		000040000000;
static const unsigned long BUFFER_FORCE=				010000000000;	/* Force the selected mode (otherwise it selects a beter one) */
static const unsigned long DUMMY_LOAD=				020000000000;	/* ALLOCATE NOTHING AT ALL (used to check needed memory) */
static const unsigned long MAX_NB_PRESEL_MASK=		000000000777;	/* MAX NB Presel used to fix maximum Runtime Memory (1 to 511) */
static const unsigned long DEFAULT_BUFFER_LEVEL=(BUFFER_UNITACOUSTIC_ALL | BUFFER_UNITSOFFSETS_ALL|BUFFER_BERSTREAM_ALL|MAX_NB_PRESEL_MASK|BUFFER_FORCE|BBSELECTOR_DEFAULT_MAX_NB_PRESEL);
/* better: 50|BUFFER_UNITACOUSTIC_ALL|BUFFER_UNITSOFFSETS_SMALL|BUFFER_BERSTREAM_ALL */
#else
/*const uint32 BUFFER_UNITACOUSTIC_SMALL=	001000000000; Not available*/
#define BUFFER_UNITACOUSTIC_MED	002000000000
#define BUFFER_UNITACOUSTIC_ALL	004000000000
#define BUFFER_UNITSOFFSETS_SMALL 000100000000
#define BUFFER_UNITSOFFSETS_ALL	000400000000
#define BUFFER_BERSTREAM_SMALL	000010000000
#define BUFFER_BERSTREAM_ALL	000040000000
#define BUFFER_FORCE			010000000000	/* Force the selected mode (otherwise it selects a beter one) */
#define DUMMY_LOAD				020000000000	/* ALLOCATE NOTHING AT ALL (used to check needed memory) */
#define MAX_NB_PRESEL_MASK		000000000777	/* MAX NB Presel used to fix maximum Runtime Memory (1 to 511) */
#define DEFAULT_BUFFER_LEVEL   (BUFFER_UNITACOUSTIC_ALL | BUFFER_UNITSOFFSETS_ALL|BUFFER_BERSTREAM_ALL|MAX_NB_PRESEL_MASK|BUFFER_FORCE|BBSELECTOR_DEFAULT_MAX_NB_PRESEL)
/* better: 50|BUFFER_UNITACOUSTIC_ALL|BUFFER_UNITSOFFSETS_SMALL|BUFFER_BERSTREAM_ALL */
#endif

static const int SELECTOR_NBMEMREC= 5;  /* Max nb_targets in a _ to _ chunk */
static const int SELECTOR_MEMREC_INDEX_AUDIO=		0;
static const int SELECTOR_MEMREC_INDEX_NUUL	=		1;
static const int SELECTOR_MEMREC_INDEX_ENGINE	=	2;
static const int SELECTOR_MEMREC_INDEX_AUDIO_2	=	3;
static const int SELECTOR_MEMREC_INDEX_NUUL_2	=	4;	

#define BBSELECTOR_SYNCONPHONEMES  0x0001		/* sync on ponemes */
#define BBSELECTOR_SYNCONWORDS	   0x0002		/* sync on words */
#define BBSELECTOR_SYNCONBREATHG   0x0004		/* sync on breath groups */
#define BBSELECTOR_SYNCONSENTENCE  0x0008		/* sync on sentences */
#define BBSELECTOR_SYNCONDURATION  0x0010		/* pass duration information for animated face! */
#define BBSELECTOR_SYNCONDIPHONES  0x0020		/* sync on diphones */
#define BBSELECTOR_PRESEL_MODE     0x2000		/* Debug mode !!! Output preselected units*/
#define BBSELECTOR_INDEX_MODE      0x8000		/* Debug mode !!! Output selected units*/
#define BBSELECTOR_SYNCSENT_FLAG   0x4000		/* status FLAG used to remember that the SYNC point was already reached
												=> set is reached, unset just after SYNC point */
/* ******************* */
/* Callback constants */
#define BBSELECTOR_SYNCPHOCALL		1			/* sync on ponemes */
#define BBSELECTOR_SYNCWORCALL		2			/* sync on words */
#define BBSELECTOR_SYNCBRECALL		3			/* sync on breath groups */
#define BBSELECTOR_SYNCSENCALL		4			/* sync on breath sentences */
/* SYNCTEXTCALL						5			-> BABILE */
/* SYNCEOTCALL						6			-> BABILE */
#define BBSELECTOR_SYNCDURCALL		7			/* return phoneme durations at once for the breath group */
#define BBSELECTOR_SYNCDIPCALL		8			/* sync on Diphones */

#define BBSELECTOR_PARM_NBPRESEL	1
#define BBSELECTOR_PARM_PHOCONTEXT	2
#define BBSELECTOR_PARM_PITCHRATIO	3
#define BBSELECTOR_PARM_SPEEDRATIO	4
#define BBSELECTOR_PARM_VOICEFREQ	5
#define BBSELECTOR_PARM_VOICECTRL   6
#define BBSELECTOR_PARM_VOICESHAPE	7           /* Removed -> now external module (BABILE) */
#define BBSELECTOR_PARM_TIMERATIO	8			/* = 10000/BBSELECTOR_PARM_SPEEDRATIO */
#define BBSELECTOR_PARM_SELBREAK	9			/* 0 -> No sub breath groups; >0 -> minimal length of the sub-breath groups */
#define BBSELECTOR_PARM_UNITSVALIDITYCALLBACK	10
#define BBSELECTOR_PARM_SELECTUNIT				11 /* (Harmoniser) select this unit id for future use */
#define BBSELECTOR_PARM_MAKENUUT				12 /* (Harmoniser) generates NUUT and WAV using selected units (by BBSELECTOR_PARM_SELECTUNIT)*/
#define BBSELECTOR_PARM_RESETSELECTEDUNITS		13 /* (Harmoniser) reset selected units (by BBSELECTOR_PARM_SELECTUNIT) */
#define BBSELECTOR_PARM_RESERVED4				14 /* (Harmoniser) not used yet*/
#define BBSELECTOR_PARM_SPANAROUNDVOICEPITCHDELTA 15 /* (Picola optimisation)(special, hidden) Allow to limit pitch lookup arround theoritical phonemùe pitch span
														can be positive or negative. lookup span is pitch delta (of phoneme) + span	*/

/* defines MIN and MAX boundaries for every setting */
#define BBSELECTOR_PARM_NBPRESEL_MIN 1
#define BBSELECTOR_PARM_NBPRESEL_MAX 200
#define BBSELECTOR_PARM_PHOCONTEXT_MIN 0
#define BBSELECTOR_PARM_PHOCONTEXT_MAX 5
#define BBSELECTOR_PARM_PITCHRATIO_MIN	10
#define BBSELECTOR_PARM_PITCHRATIO_MAX 500
#define BBSELECTOR_PARM_SPEEDRATIO_MIN 25
#define BBSELECTOR_PARM_SPEEDRATIO_MAX 400
#define BBSELECTOR_PARM_TIMERATIO_MIN	25
#define BBSELECTOR_PARM_TIMERATIO_MAX	400
#define BBSELECTOR_PARM_SELBREAK_MIN	0
#define BBSELECTOR_PARM_SELBREAK_MAX	200
#define BBSELECTOR_PARM_VOICESHAPE_MIN	25
#define BBSELECTOR_PARM_VOICESHAPE_MAX	400
#define BBSELECTOR_PARM_SPANAROUNDVOICEPITCHDELTA_MIN -1000
#define BBSELECTOR_PARM_SPANAROUNDVOICEPITCHDELTA_MAX 1000

#endif /* __ACA__BB__IC_SELECTOR_H__ */
