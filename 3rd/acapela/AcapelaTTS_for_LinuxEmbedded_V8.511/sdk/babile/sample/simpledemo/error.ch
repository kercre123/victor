#include "is_babile.ch"
#include "is_nlpe.h"
#include "is_bbnlp.h"
#include "is_bbsel.h"
#include "is_mbre.h"
#include "is_colibri.h"

#ifndef BBABS
# define BBABS(X) (X<0?-(X):X)
#endif 
static int testInitParam(BABILE_MemParam* initializationParameters,BB_S32 synthActive,FILE* FILEOUT)
{
		if (!initializationParameters->nlpeLS)	{fprintf(FILEOUT,"NO NLP DATA!\n"); return -1000;}
		if (!initializationParameters->synthLS)	{fprintf(FILEOUT,"NO VOICE DATA!\n"); return -1000;}
		if (!initializationParameters->nlpeLS && !initializationParameters->synthLS) {fprintf(FILEOUT,"DATA LOAD ERROR!\n"); return -1000;}
		else
		{
			if (initializationParameters->nlpModule==BABILE_NLPMODULE_NLPE) fprintf(FILEOUT,"NLPE");
			if (initializationParameters->nlpModule==BABILE_NLPMODULE_TTF) fprintf(FILEOUT,"IVXTTF");
			fprintf(FILEOUT,"\n");
			if (initializationParameters->synthModule&BABILE_SYNTHMODULE_MBROLA) fprintf(FILEOUT,"MBROLA Loadable ");
			if (initializationParameters->synthModule&BABILE_SYNTHMODULE_BBSELECTOR) fprintf(FILEOUT,"SELECTOR Loadable ");
			if (initializationParameters->synthModule&BABILE_SYNTHMODULE_COLIBRI) fprintf(FILEOUT,"COLIBRI Loadable ");
			if ((synthActive&BABILE_SYNTHACTIVE_MASK)==BABILE_SYNTHACTIVE_MBROLA) fprintf(FILEOUT,"->MBROLA");
			if ((synthActive&BABILE_SYNTHACTIVE_MASK)==BABILE_SYNTHACTIVE_BBSELECTOR) fprintf(FILEOUT,"->SELECTOR");
			if ((synthActive&BABILE_SYNTHACTIVE_MASK)==BABILE_SYNTHACTIVE_COLIBRI) fprintf(FILEOUT,"->COLIBRI");
			fprintf(FILEOUT,"\n");
		}
		return 0;
}

static int testError(BABILE_Obj* bab,BABILE_MemParam* initializationParameters,FILE* FILEOUT)
{	/* print error messages */
	BB_ERROR err=0,eb=0,em=0,en=0;
	if (initializationParameters && !bab)
	{
		eb=initializationParameters->initError;
	}
	else
	{	
		 eb=BABILE_getError(bab,&em,&en);
	}
	err=eb;
	if (eb)
	{
		fprintf(FILEOUT,"\nERROR " BB_ERROR_PFMT, eb);
		eb=BBABS(eb);
		if (eb<=BABILE_MAX_ERROR_VALUE)
		{
			fprintf(FILEOUT," %s\n",BABILE_errorStrings[eb&0xFF]);
		}
		if (initializationParameters)
		{
			BB_ERROR es=0,ec=0;
			em=initializationParameters->mbrInitError;
			en=initializationParameters->nlpInitError;
			es=initializationParameters->selInitError;			
			ec=initializationParameters->colInitError;			
			fprintf(FILEOUT,"-> (INIT; MBR " BB_ERROR_PFMT ", SEL " BB_ERROR_PFMT ", COL " BB_ERROR_PFMT ", NLP " BB_ERROR_PFMT ")",em,es,ec,en);
			em=BBABS(em);
			en=BBABS(en);
			es=BBABS(em);
			ec=BBABS(ec);
			if (em && em<=BBSEL_MAXERROR_VALUE_STRING)
			{
				fprintf(FILEOUT," %s\n",BBSEL_errorStrings[em&0xFF]);
			}	
			if (es && es<=BBSEL_MAXERROR_VALUE_STRING)
			{
				fprintf(FILEOUT," %s\n",BBSEL_errorStrings[es&0xFF]);
			}
			if (ec) 
			{
				fprintf(FILEOUT," %s\n",ALF_ReturnCode_2str((ec&0xFF)));
			}			
		}
		if (bab)
		{
			BB_SPTR parm=0;
			fprintf(FILEOUT,"-> (SYNTH " BB_ERROR_PFMT", NLP " BB_ERROR_PFMT")",em,en);
			em=BBABS(em);
			en=BBABS(en);
			if (BABILE_getSetting(bab, BABIL_PARM_ACTIVEMODULES,&parm)>=0)
			{
				if (em)
				{
					if ((parm&BABILE_SYNTHACTIVE_MASK)==BABILE_SYNTHACTIVE_BBSELECTOR) 
					{
						if (em<=BBSEL_MAXERROR_VALUE_STRING)
						{
							fprintf(FILEOUT," %s\n",BBSEL_errorStrings[em&0xFF]);
						}
					}
					if ((parm&BABILE_SYNTHACTIVE_MASK)==BABILE_SYNTHACTIVE_COLIBRI) 
					{
							fprintf(FILEOUT," %s\n",ALF_ReturnCode_2str((em&0xFF)));
					}						
					if ((parm&BABILE_SYNTHACTIVE_MASK) ==BABILE_SYNTHACTIVE_MBROLA   ) 
					{
						if (em<=MBRE_MAXERROR_VALUE_STRING)
						{
							fprintf(FILEOUT," %s\n",MBRE_errorStrings[em&0xFF]);
						}
					}
				}
				if (en)
				{
					if ((parm&BABILE_NLPACTIVE_TTF)==BABILE_NLPACTIVE_NLPE) 
					{
						if (en<=NLPE_MAX_ERROR_VALUE)
						{
							fprintf(FILEOUT," %s\n",NLPE_errorString[en&0xFF]);
						}
					}
					if ((parm&BABILE_NLPACTIVE_TTF) ==BABILE_NLPACTIVE_TTF   ) 
					{
					}
				}
			}
		}
	}
	BABILE_resetError(bab);
	return (int) err;
}
