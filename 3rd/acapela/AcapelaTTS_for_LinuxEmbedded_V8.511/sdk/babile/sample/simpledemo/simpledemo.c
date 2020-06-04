#include "i_babile.h" /* Includes All Main BABILE API headers */
#include <string.h>					
#include <stdio.h>
#include <stdlib.h>

/* Add some helper functions ( All the code below is optinal)*/
/* Helper functions vvvv */
#include "error.ch"     /* For error code interpretation */
#define USE_LOADINIFILE
#include "dblsman.ch"	/* BABEL-Acapela's ini files reader */
#include "audio.ch"		/* Audio output */
/* Helper functions ^^^^ */

#define OUTBUFFER_SIZE	2048  /* This is the minimal out buffer size */

int main(int argc, char** argv)
{
	BB_S16 outBuffer[OUTBUFFER_SIZE];
	char const * iniFileName;
	char const * textFile;
	BB_TCHAR* text;
	char * output;
	int outHndl=0;
	BB_SPTR freq=16000;
	BB_SPTR samplesize=2;
	BB_U32 totalSamples=0;							
	const char* overrideLoad="*=RAM";												
	int err=0;
/* License handling: It is possible to link staticaly the licence data */
#ifdef MY_LICENSE_H
# include MY_LICENSE_H  /* INCLUDE YOUR LICENSE FILE It defines babLicense global variable*/
# include MY_LICENSE_UID /* USE YOUR LICENSE ID & PASSWORD It defines uis global structure*/
#endif
	BABILE_Obj* bab=NULL;
	BB_DbLs* pData=NULL; /* pointer to the data */


	{  /* Check command line paramaters */

		if (argc<4)
		{
			printf("SYNTHAX: %s <ini_file> <input_text> <output>\n",argv[0]);
			return -1000;
		}
		iniFileName=argv[1];
		textFile=argv[2];
		output=argv[3];

		text=DumpStringFileStatic(textFile,NULL);
		if (!text) {printf ("ERROR: Can't read text file: %s\n",textFile);exit (-1);}
	}

	{/* INIITIALISATION STEP */
		BB_DbLs* pNLPLs=NULL;
		BB_DbLs* pSynthLs=NULL;
		short nlpActif=0;
		short synthActif=0;
		BB_S32  synthAvailable=0;
		char* defaultText=NULL;
		BABILE_MemParam initializationParameters;
		memset(&initializationParameters,0,sizeof(BABILE_MemParam));
		/* A) Compulsory initialization parameters*/
		initializationParameters.sSize=sizeof(BABILE_MemParam); /* Sanity+version check*/
		/* A-1) License*/
		initializationParameters.license=babLicense;	/* your license (defined in MY_LICENSE_H) */
		initializationParameters.uid.passwd=uid.passwd;	/* your password (defined in MY_LICENSE_UID) */
		initializationParameters.uid.userId=uid.userId; /* your ID (defined in MY_LICENSE_UID) */
		/* A-2) Data to use (retrieved from the .ini file) */
		pData=BABILE_loadIniFile(iniFileName,&pNLPLs,&pSynthLs,&nlpActif,&synthAvailable,&synthActif,(const char**) &defaultText,overrideLoad);
		if (!pData)	{printf("DATA LOAD ERROR!\n"); return -1000;}
		initializationParameters.nlpeLS=pNLPLs;
		initializationParameters.nlpModule=nlpActif;
		initializationParameters.synthLS=pSynthLs;
		initializationParameters.synthModule=synthAvailable;
		/**/
		bab=BABILE_initEx(&initializationParameters);
		/* D) those variables will contain initialization errors 
		initializationParameters.initError;
		initializationParameters.selInitError
		initializationParameters.nlpInitError
		initializationParameters.mbrInitError 
		*/
		if ((err=testError(bab,&initializationParameters,stdout))<0)
		{
			goto CLEAN;
		}
		if (bab)
		{
			char version[255];
			*version=0;
			printf("INITIALIZATION OK %s\n",BABILE_getVersion());
			BABILE_getVersionEx(bab,version,sizeof(version)/sizeof(char));
			printf("%s\n",version);
		}
		else
		{
			goto CLEAN;
		}
	}
	if (!strncmp(text,BOM_STR_UTF8,3))
	{/* TEXT is UTF8 */
		if (BABILE_setSetting(bab,BABIL_PARM_CODEPAGE,65001)<0)
		{
			printf("This language doesn't support UTF8\n");
		}
	}
	BABILE_getSetting(bab,BABIL_PARM_VOICEFREQ,&freq);
	BABILE_getSetting(bab,BABIL_PARM_SAMPLESIZE,&samplesize);
	outHndl=open_audio(output,(int)freq);
	
	if (bab && text)
	{/* PROCESS */
		BB_U32 nbSamples=0;
		BB_S32 charRead=0;
		int startPos=0;
		do
		{
			charRead=BABILE_readText(bab,&text[startPos],outBuffer,(BB_U32)(sizeof(outBuffer)/samplesize),&nbSamples);
			if (charRead>=0 && nbSamples>0)		
			{  /* Writes the samples to audio */
				totalSamples+=nbSamples;
				if (outHndl) write(outHndl,outBuffer,(unsigned int)(nbSamples*samplesize));
			}
			if (charRead>0)										
			{
				startPos+=charRead;
			}
		} while (charRead>0 || (charRead>=0 && nbSamples>0));
		do
		{
			BABILE_readText(bab,NULL /* == FLUSH INTERNAL BUFFERS */,outBuffer,(BB_U32)(sizeof(outBuffer)/samplesize),&nbSamples);
			if (nbSamples>0) 
			{  /* Writes the samples to audio */
				totalSamples+=nbSamples;
				if (outHndl) write(outHndl,outBuffer,(unsigned int)(nbSamples*samplesize));
			}
		} while (nbSamples>0);
		if (testError(bab,NULL,stdout)>=0) printf("NO ERROR\n");
		printf("Generated %d samples",   totalSamples);
		printf("\n");
	}
	close_audio(outHndl,output,totalSamples);

	{/*		cleaning*/
CLEAN:
		BABILE_freeEx(bab);
		destroyLanguageDba(pData);
		if (text) free(text);
	}
	return err;

}
