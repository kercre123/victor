#include "ic_babile.h"
#include "dblsman.h"
 
#if defined(__SYMBIAN32__)
#include "filesys.hpp"
#include "bbansi.hpp"

#define	SSCANF			TBBANSI::BB_sscanf
#define STRDUP			strdup
#define PRINTF0(X)		TBBANSI::printf(X)
#define PRINTF1(X,Y)		TBBANSI::printf(X,Y)
#define FTELL(A)		CBbFile::ftell(A) 
#define FSEEK(A,B,C)	CBbFile::fseek(A,B,C)
#define FREAD(A,B,C,D)	CBbFile::fread((TUint8*)A,B*C,D)
#define FGETS(A,S,F)	(char*)(CBbFile::fgets((TUint8*)A,S,F))
#define FOPEN(A,B)		CBbFile::fopen(A,B,aFs)
#define FCLOSE(F)		CBbFile::fclose(F)
#define FSCLOSE			CleanupStack::PopAndDestroy((TAny *)&aFs)
#define FSOPEN          RFs aFs; if (aFs.Connect()!=KErrNone) return 0; CleanupClosePushL(aFs);
#define MALLOC(A)		User::Alloc(A)
#define FREE(A)			User::Free(A)
#define _FILE			RFile
#define BB_FILENAME		TDesC
#define BBTCHAR			TUint8
#define BBLENGTH(X)		X->Length()
#define READBINARY		"rb"
#elif defined(__IAR_EWB__)
#ifdef HCC_EMBEDDED
#define FTELL(A)		f_tell(A)
#define FSEEK(A,B,C)	        f_seek(A,B,C)
#define FREAD(A,B,C,D)	        f_read(A,B,C,D)
#define FGETS(A,S,F)	        f_gets((char*)A,S,F)
#define FOPEN(A,B)		f_open((const char*)A,B)
#define FCLOSE(F)		f_close(F)
#define FSCLOSE
#define FSOPEN
#define MALLOC(A)		malloc(A)
#define FREE(A)			free(A)
#define _FILE			F_FILE
#define BB_FILENAME		BBTCHAR 
#define BBTCHAR			char
#define PRINTF0(X)			__printf0(X)
#define PRINTF1(X,Y)			__printf1(X,Y)
#define STRDUP			str_dup
#define	SSCANF			sscanf
static int f_gets(char* buffer,int size, F_FILE *f)
{
  int i = 0, ch;
  int found = 0;
  static int backup_ch = (int)'\r';
  
  if ((char)backup_ch != '\r') {
    *buffer++ = (char)backup_ch;
    i++;
    backup_ch = (int)'\r';
  }
  while ((!found) && ((ch = f_getc(f)) != -1) && (i<size-1)) {
    if (((char)ch == '\r') || ((char)ch == '\n')) {
      while (((char)ch == '\r') || ((char)ch == '\n')) {
        found = 1;
        ch = f_getc(f);
        if (!(((char)ch == '\r') || ((char)ch == '\n'))) {
          backup_ch = ch;
        }
      }
    } else {
      *buffer++ = (char)ch;
      i++;
    }
  }
  *buffer = '\0';
  return i;
}
static char* str_dup(const char* str)
{
  char *retv = NULL;
  int size = strlen(str)+1;
  retv = (char*)malloc(size);
  if (retv)
    memcpy(retv,str,size);
  return retv;
}
#define BBLENGTH(X)		strlen(X)
#define READBINARY		"r"
#endif /* HCC_EMBEDDED */
#else
#define FTELL(A)		ftell(A)
#define FSEEK(A,B,C)	fseek(A,B,C)
#define FREAD(A,B,C,D)	fread(A,B,C,D)
#define FGETS(A,S,F)	fgets((char*)A,S,F)
#define FOPEN(A,B)		fopen((const char*)A,B)
#define FCLOSE(F)		fclose(F)
#define FSCLOSE
#define FSOPEN
#define MALLOC(A)		malloc(A)
#define FREE(A)			free(A)
#define _FILE			FILE
#define BB_FILENAME		BBTCHAR 
#define BBTCHAR			char
#define PRINTF0(X)			printf(X)
#define PRINTF1(X,Y)			printf(X,Y)
#define PRINTF2(X,Y,Z)			printf(X,Y,Z)
#define STRDUP			strdup
#define	SSCANF			sscanf
#define BBLENGTH(X)		strlen(X)
#define READBINARY		"rb"
#endif

static BBTCHAR* DumpFileStatic(_FILE* f, BB_S32* size)
{
	long pos1,pos2;
	BBTCHAR* dump;
	pos1=FTELL(f);
	FSEEK(f,0,SEEK_END);
	pos2=FTELL(f);
	pos2-=pos1;
	if (pos2<0) return 0;
	dump=(BBTCHAR*) MALLOC(pos2+1);	/* TO store the EOF to */
	if (!dump)
	{
		FSEEK(f,pos1,SEEK_SET);
		return 0;
	}
	FSEEK(f,pos1,SEEK_SET);
	pos2= FREAD(dump,1,pos2,f);
	if (size) *size=pos2;
	dump[pos2]=(BBTCHAR) 0;		/* putting an EOF */
	return dump;
}

static BBTCHAR* DumpStringFileStatic(const BBTCHAR* const fichier, BB_S32* size)
{
	_FILE* fich;
	BBTCHAR *dump;
	
	FSOPEN;
	fich = FOPEN(fichier, READBINARY);
	if (!fich)
	{
		FSCLOSE;
		return 0;
	}
	dump =DumpFileStatic(fich, size);
	FCLOSE(fich);
	FSCLOSE;
	return dump;
}

static long GetFileSizeStatic(const BBTCHAR* const fichier)
{
	_FILE* fich;
	long len=0;
	FSOPEN;
	fich = FOPEN(fichier, READBINARY);
	if (!fich)
	{
		FSCLOSE;
		return 0;
	}
	FSEEK(fich,0,SEEK_END);
	len=FTELL(fich);
	FCLOSE(fich);
	FSCLOSE;
	return len;
}

int fillInitId(BB_DbLs* dbLsBlk, const BBTCHAR *Path,const BBTCHAR* descriptor,BB_DbMemType type,BB_U8 swapEndian)
{
	BB_S32 size=-1;
	dbLsBlk->pDbId=(BB_DbId*) MALLOC(sizeof(BB_DbId));
	if (!dbLsBlk->pDbId)
	{
		PRINTF0("Not Enough Memory!\n");
		return -1;
	}
	memset(dbLsBlk->pDbId,0,sizeof(BB_DbId));
#ifdef FILE_ACCESS_MEMORY
	dbLsBlk->pDbId->type=type;
#if defined (_UNICODE) && !defined(__SYMBIAN32__)
	if ((type&(BB_DBTYPE_FILE|BB_DBMEM_MEMORYMAP))  
		&& ((dbLsBlk->pDbId->link=(BB_TCHAR*)_strdup(Path))==0))
#else
	if ((type&(BB_DBTYPE_FILE|BB_DBMEM_MEMORYMAP))
		&& ((dbLsBlk->pDbId->link= (BB_TCHAR*) STRDUP((const char*)Path))==0))
#endif 
	{
		PRINTF1("%s\n",(const char*)dbLsBlk->pDbId->link);
		PRINTF1("%s\n",(const char*)Path);
		FREE(dbLsBlk->pDbId);
		return -1;
	}
	if ((type&(X_RAM_MASK|BB_DBMEM_MEMORYMAP))==X_RAM)
	{
		if ((dbLsBlk->pDbId->link=(BB_TCHAR*) DumpStringFileStatic((const BBTCHAR*) Path, &size))==0)
		{
			PRINTF1("%s\n",(const char*)Path);
			FREE(dbLsBlk->pDbId);
			return -1;
		}
	}
#else
	dbLsBlk->pDbId->type=X_ONLY_RAM;
    if ((dbLsBlk->pDbId->link=(BB_TCHAR*) DumpStringFileStatic((BBTCHAR*) Path, NULL))==0)
	{
		PRINTF1("%s\n",Path);
		free(dbLsBlk->pDbId);
		return -1;
	}
#endif
	dbLsBlk->pDbId->size=GetFileSizeStatic((const BBTCHAR*) Path);
	if (swapEndian!=0) dbLsBlk->pDbId->flags=BB_DBFLAGS_INVERSE;
	strncpy((char*)dbLsBlk->descriptor, (const char*)descriptor, 4);
	return 0;
}

int fillInitStruct(const BBTCHAR* line, BB_DbLs* dbLsBlk, const BBTCHAR* accessPath, const BBTCHAR* descriptor, BB_DbMemType type, BB_U8 swapEndian)
{	/* this function fills the BB_DbLs structure */
	BBTCHAR fullPath[512];
	fullPath[0]=0;
	if (!line) return -1;
    if(accessPath)
    {
		int l=(int) strlen((const char*)accessPath);
		l=(l>0?l-1:0);
        strcpy((char*)fullPath, (const char*)accessPath);
        if (fullPath[l]=='\\' || fullPath[l]=='/')
        {
			fullPath[l]='/';l++;
			fullPath[l]=0;
		}
	}
	strcat((char*)fullPath, (const char*)line);
#ifdef 	PLATFORM_LINUX
	{
		unsigned int i;
		for (i=0;i<strlen(fullPath);i++) {if (fullPath[i]=='\\') {fullPath[i]='/';}}
	}
#endif
	return fillInitId(dbLsBlk,fullPath,descriptor,type,swapEndian);
}

void destroyInitStruct(BB_DbLs* dbLsBlk)
{
	if (dbLsBlk)
	{
		if (dbLsBlk->pDbId)
		{
			if (dbLsBlk->pDbId->link)
			{
				FREE(dbLsBlk->pDbId->link);
			}
			FREE(dbLsBlk->pDbId);
			dbLsBlk->pDbId=NULL;
		}
	}
}

void destroyLanguageDba(BB_DbLs* ldba)
{
	int i;
	if (ldba)
	{
		for (i=0; ldba[i].descriptor[0]; i++)
		{
			destroyInitStruct(&ldba[i]);
		}
		FREE(ldba);
	}
}

int initLanguageDbaNewAddin(BB_DbLs* p_nlpDba_i,BBTCHAR* line,BBTCHAR* decl,BB_DbMemType useType,BB_U8 swapEndian,BBTCHAR* accesspath,  const char* userdatapath)
{
	if (strcmp(userdatapath,"") != 0) 
	{
		BBTCHAR filename[512];
        BBTCHAR filepath[512];
        FILE * currentfile;
                
        // First test if the file exists in voice folder path + file path given in the ini
        sprintf(filepath,"%s/%s",accesspath,line);    
        currentfile = fopen(filepath,"rb");
        if (currentfile == NULL) {

			//Second test if the file exists in userdatapath + file path given in the ini
			sprintf(filepath,"%s/%s",userdatapath,line);
	
			currentfile = fopen(filepath,"rb");
        	if (currentfile != NULL) {
        		fclose(currentfile);
    		
			if (userdatapath[strlen(userdatapath)] == '/')
				sprintf(filename,"%s",line);
			else
				sprintf(filename,"/%s",line);

			if (fillInitStruct(filename,p_nlpDba_i,userdatapath,decl,useType,swapEndian)<0) {
				return 0;
			}
		} else {
                                
			// Third test is file exists in userdatapath + filename only given in the ini
			int separator = 0;
			for(separator=(int)strlen(line); separator>0; separator--)
			if ((line[separator]=='\\') || (line[separator]=='/'))
				break;

			//if not check if the file exists in the userdatapath set in the application
			if (userdatapath[strlen(userdatapath)] != '/' && line[separator] != '/')
				sprintf(filename,"/%s",line+separator);
			else
				sprintf(filename,"%s",line+separator);
									
				sprintf(filepath,"%s/%s",userdatapath,filename);
				currentfile = fopen(filepath,"rb");
        		if (currentfile != NULL) {
        			fclose (currentfile);
        		}
				
				if (fillInitStruct(filename,p_nlpDba_i,userdatapath,decl,useType,swapEndian)<0) {
					return 0;
				}
			}
        } else {
            fclose(currentfile);
            if (fillInitStruct(line,p_nlpDba_i,accesspath,decl,useType,swapEndian)<0) {
                return 0;
            }
		}
    
    } else if (fillInitStruct(line,p_nlpDba_i,accesspath,decl,useType,swapEndian)<0) {
		return 0;
	}
	return 1;
}


/* defualt loadstring:
"*=RAM,BNF=+RAM,IVX=+RAM,PHM=+RAM,MODE=default|no_smiley"
*/
#define LOADTYPESIZE	32
#define PREFIXSIZE		8

static void getTypeToUse(const char* const decl, char*  strType, const char* const loadconfig,int* swapEndian, int* useType,int* force)
{
	const char* findloadconf=NULL;
	char strdecl[PREFIXSIZE+1]; /* to store additionnal '=' */
	int ut=0;
	strncpy(strdecl,decl, PREFIXSIZE);
	strcat(strdecl,"=");
	if ((findloadconf=strstr(loadconfig,strdecl))!=NULL)
	{
		findloadconf+=strlen(strdecl);
		if (force) *force=0;
		if (*strType==0 || *findloadconf=='+')
		{
			int xix=0;
			if (swapEndian) *swapEndian=0;
			if (*findloadconf=='+')
			{
				if (force) *force=1;
				findloadconf++;
			}
			for (xix=0;*findloadconf && *findloadconf!=',' && *findloadconf!=' ' && xix<LOADTYPESIZE-1;xix++,findloadconf++)
			{
				strType[xix]=*findloadconf;
			}
			strType[xix]=0;
		}
	}
	if ((ut=atoi(strType))>0) 
	{
		*useType=ut;
	}
	else
	{
		if (!strncmp(strType,"ENDIAN_",7)) { if (swapEndian) *swapEndian=1; strType+=7; }
		if (!strncmp(strType,"RAM",3)) {*useType=X_RAM;strType+=3;}
		else if (!strncmp(strType,"FILEMAP",7)) {*useType=X_FILEMAP;strType+=7;}
		else if (!strncmp(strType,"FILE",4)) {*useType=X_FILE;strType+=4;}
		if (!strncmp(strType,"_PROTECTED",10)) {*useType|=BB_DBSIZEPROTECT_ON;strType+=10;}
	}
}

static int getPossibleModes( char const* const modes,  char const * const requested)
{ /* Max mode Number is (sizeof(int)*8)-1 */
	char const *  foundMode;
	char const * _modes=modes;
	char strType[LOADTYPESIZE];
	int imode=0;
	int nbm=0;
	const char* findloadconf=NULL;
	if ((findloadconf=strstr(requested,"MODE="))!=NULL)
	{
		char const * req=findloadconf+5;
		while (req && *req )
		{
			int xix=0;
			for (xix=0;*req && *req!=',' && *req!='|' && *req!=' ' && xix<LOADTYPESIZE-1;xix++,req++)
			{
				strType[xix]=*req;
			}
			strType[xix]=0;
			if (*req==',') break;
			if (*req){req++;}
			if ((foundMode=strstr(_modes,strType))!=NULL)
			{
				foundMode+=strlen(strType);
				if (*foundMode==':')
				{
					foundMode++;
					if (*foundMode=='-' || *foundMode=='+') foundMode++;
					nbm=atoi(foundMode);
					if ((nbm!=0  && nbm<(sizeof(int)*8)-1) || *foundMode=='0' ) {imode|=1<<nbm;}
				}
				else
				{
					_modes=foundMode;
				}
			}
		}
	}
	return imode;
}

static int isModeValid(const char * stringmode,char mod,int validModes)
{
	int mode,ascan=2,n=0;
	int dm=0;
	char c;
	while(ascan>1 && (ascan=SSCANF(&stringmode[n],"%d%c%n",&mode,&c,&n))>=1)
	{
		if (mode<(sizeof(int)*8)-1)
		{
			dm|=1<<mode;
		}
		if (ascan>1 && c!=',') break;
	}
	if (validModes&dm && (mod==' ' || mod=='+') ) return 1;
	if (!(validModes&dm) && (mod=='-') ) return 1;
	return 0;
}

BB_DbLs* initLanguageDbaNewEx(_FILE* file,const BB_FILENAME* name,const BBTCHAR* path,int nitem,int* error,const char*const loadconfig, int (*callback) (BBTCHAR* prefix,BBTCHAR* valeur, BBTCHAR* options ,BBTCHAR* accesspath, void* object),void* obj, const char * userdatapath)
{
	_FILE* f=file;
	BB_DbLs* nlpDba;
	char strType_[LOADTYPESIZE];
	char line[256];
	char fentry[256];
	char decl[256];
	char accesspath[256];
	int i, ascan;
	int useType;
	int defaulttype=X_RAM;
	int swapEndian=0;
	int defaultswapEndian=0;
	int forceType=0;
	int defaultforceType=0;
	int imodes=0;
	char mode[100];
	*strType_=0;
	*accesspath=0;
	if (error) *error=0;
	if (!f && !name) { if (error) *error=-100;return 0;}
	if (path) strncpy((char*)accesspath,(const char*)path,sizeof(accesspath));
	if (*accesspath)
	{
#if defined(WIN32)
		if ('\\' != accesspath[strlen(accesspath) - 1])
			strncat(accesspath, "\\", (sizeof(accesspath) / sizeof(accesspath[0])) - 1);
#else
		if ('/' != accesspath[strlen(accesspath) - 1])
			strncat(accesspath, "/", (sizeof(accesspath) / sizeof(accesspath[0])) - 1);
#endif
	}
	FSOPEN;
	if ((!f) && (name!=NULL) && (BBLENGTH(name)!=0)) f=FOPEN(name,READBINARY);		/* open the ini file */
	if (!f)	
	{ 
		if (error)	*error=-1;	
		FSCLOSE;
		return 0;
	}
	/* Reading number of entries */
	if (nitem<=0)
	{
		nitem=0;
		while (FGETS(line,sizeof(line),f)!=NULL)
		{
			if (SSCANF((const char*)line,"%s \"%[^\"]\"",(char*)decl,(char*)fentry)==2 && *fentry ) nitem++;
		}
	}
	/* allocating structure */
	nlpDba=(BB_DbLs*) MALLOC(sizeof(BB_DbLs)*(nitem+1));
	if (!nlpDba) {FCLOSE(f);FSCLOSE; if (error)	*error=-2; return 0;}
	memset(nlpDba,0, sizeof(BB_DbLs)*(nitem+1));
	/* loop on init file */
	if (error) *error=-10;
	i=0;
	FSEEK(f,0,SEEK_SET);
	*strType_=0;
	getTypeToUse("*",strType_, loadconfig,&defaultswapEndian,&defaulttype,&defaultforceType); 
	while (i<nitem)
	{
		*strType_=0;
		if (error) (*error)--;
		if (FGETS((char*)line,sizeof(line),f)==NULL)
		{	
			break;
		}
		if (SSCANF((char*)line,"PATH %s \"%[^\"]\"", (char*)decl, (char*)fentry) == 2)
		{
			if (*fentry==' ') strcpy((char*)accesspath,(char*)&fentry[1]);
			else strcpy((char*)accesspath,(char*)fentry);
		}
		else if (SSCANF((char*)line,"MODE %s \"%[^\"]\"", (char*)decl, (char*)fentry) == 2 && *decl=='=')
		{
			imodes=getPossibleModes(fentry, loadconfig);
		}
		else if (SSCANF((char*)line,"%s = \"%[^\"]\"", (char*)decl, (char*)fentry) == 2)
		{
			if (callback) callback((BBTCHAR*)decl,(BBTCHAR*)fentry,NULL,(BBTCHAR*)accesspath, obj);
		}
		else if (
		((ascan=SSCANF((const char*)line,"#%s %s \"%[^\"]\" [ %[^] ] ] ",mode, (char*)decl, (char*)fentry, strType_))>=3 && isModeValid(mode,' ',imodes)) ||
		((ascan=SSCANF((const char*)line,"%s \"%[^\"]\" #%s", (char*)decl, (char*)fentry, mode))==3  && isModeValid(mode,'-',imodes)) ||
		(ascan!=3 && (ascan=SSCANF((const char*)line,"%s \"%[^\"]\" [ %[^] ] ] #%s", (char*)decl, (char*)fentry, strType_,mode))>=2 && (ascan<4 || isModeValid(mode,'-',imodes)))
		)
		{
			if (!callback || callback((BBTCHAR*)decl,(BBTCHAR*)fentry,(BBTCHAR*)strType_,(BBTCHAR*)accesspath, obj)!=0)
			{
				useType=defaulttype;
				swapEndian=defaultswapEndian;
				forceType=0;
				getTypeToUse(decl,strType_, loadconfig,&swapEndian,&useType,&forceType); 
				getTypeToUse(decl,strType_, "BNF=+RAM,IVX=+RAM,PHM=+RAM",NULL,&useType,&forceType);  /* override to force */
				if (defaultforceType && !forceType) useType=defaulttype; /* Force configuration (non optimal) */
                if (userdatapath == NULL) 
				{
                    if (fillInitStruct(fentry,&nlpDba[i],accesspath,decl,(BB_DbMemType)useType,(BB_U8)swapEndian)<0) 
					{
                        destroyLanguageDba(nlpDba);FCLOSE(f);FSCLOSE; return 0;
                    }       
                } 
				else if (!initLanguageDbaNewAddin( &nlpDba[i],fentry, decl, (BB_DbMemType)useType,(BB_U8) swapEndian, accesspath, userdatapath))
				{
					destroyLanguageDba(nlpDba);FCLOSE(f);FSCLOSE; return 0;
				}
				i++;
			}
		}
	}
	if (error) *error=0;
	FCLOSE(f);
	FSCLOSE;
	return nlpDba;
}
BB_DbLs* initLanguageDbaNew(_FILE* file,const BB_FILENAME* name,const BBTCHAR* path,int nitem,int* error,unsigned short type,int forceType,  int (*callback) (BBTCHAR* prefix,BBTCHAR* valeur, BBTCHAR* options ,BBTCHAR* accesspath, void* object),void* obj, const char * userdatapath)
{
	char strType[LOADTYPESIZE];
	int i=0;
	*strType=0;
	if (type)
	{
		strType[i]='*';i++;
		strType[i]='=';i++;
		if (forceType) {strType[i]='+';i++;}
		strType[i]=0;
		sprintf(&strType[i], "%d", type);
	}
	return initLanguageDbaNewEx(file, name, path, nitem,error, strType, callback, obj,userdatapath);
}
#undef LOADTYPESIZE
#undef PREFIXSIZE

BB_DbLs* initLanguageDbaExEx(_FILE* file,const BB_FILENAME* name,const BBTCHAR* path,int nitem,int* error,unsigned short type,int forceType, int (*callback) (BBTCHAR* prefix,BBTCHAR* valeur, BBTCHAR* options ,BBTCHAR* accesspath, void* object),void* obj)
{
	return initLanguageDbaNew( file, name, path, nitem,error, type, forceType, callback, obj, NULL );
}

BB_DbLs* initLanguageDbaEx(_FILE* file,const BB_FILENAME* name,const BBTCHAR* path,int nitem,int* error,unsigned short type,int forceType)
{
	return initLanguageDbaExEx( file,name, path, nitem, error,type, forceType,NULL,0);
}

BB_DbLs* initLanguageDba(_FILE* file,const BB_FILENAME* name,const BBTCHAR* path,int nitem,int* error,unsigned short type)
{
	return initLanguageDbaEx(file,name,path,nitem,error,type,0);
}

#undef FTELL	
#undef FSEEK
#undef FREAD
#undef FOPEN	
#undef FCLOSE	
#undef MALLOC	
#undef FREE		

#ifndef __SYMBIAN32__
#ifdef USE_LOADINIFILE
static BB_DbLs* BABILE_loadIniFile( char const * const aFilename,BB_DbLs** ppNLPLs,BB_DbLs** ppSynthLS,short* nlpModule,BB_S32* synthAvailable,short* synthActif, const char** pText, const char*const loadParams )
{
	int error=0,i;
	BB_DbLs* iData=NULL;
	char  * chainePath;
	if (!aFilename || !ppNLPLs || !ppSynthLS || !nlpModule || !synthAvailable || !synthActif || !pText)
	{
		return NULL;
	}
	/* GET PATH */
#if defined (_UNICODE) && !defined(__SYMBIAN32__)
	chainePath=_strdup(aFilename);
#else
	chainePath=strdup(aFilename);
#endif
	if (chainePath)
	{
		for (i=(int) strlen(chainePath); i>=0; i--)
		{
			if ((chainePath[i]=='\\') ||
				(chainePath[i]=='/'))
				break;
		}
		chainePath[i+1]=0;
	}
	iData=initLanguageDbaNewEx(NULL,aFilename,chainePath,0,&error,loadParams,NULL,NULL,NULL);
	if (chainePath) free(chainePath);
	if (iData)
	{
		int vi;
		BB_DbLs* voiceData=0;
		*synthActif=BABILE_SYNTHACTIVE_NONE;
		*synthAvailable=0;
		*nlpModule=BABILE_NLPMODULE_NLPE;
		/* CHECK for MBROLA */
		for (vi=0; iData[vi].descriptor[0] && strcmp((char*)iData[vi].descriptor,"MBR") ;vi++);
		if (iData[vi].descriptor[0])
		{
			voiceData=&iData[vi];
			*synthActif=BABILE_SYNTHACTIVE_MBROLA;
			*synthAvailable|=BABILE_SYNTHMODULE_MBROLA;
		}
		/* CHECK for SELECTOR */
		for (vi=0; iData[vi].descriptor[0] && (
				strcmp((char*)iData[vi].descriptor,"SLV") &&
				strcmp((char*)iData[vi].descriptor,"SLL")
				) ;vi++);
		if (iData[vi].descriptor[0])
		{
			voiceData=&iData[vi];
			*synthActif=BABILE_SYNTHACTIVE_BBSELECTOR;
			*synthAvailable|= BABILE_SYNTHMODULE_BBSELECTOR;
		}
		/* CHECK for COLIBRI */
		for (vi=0; iData[vi].descriptor[0] && (
				strcmp((char*)iData[vi].descriptor,"ALF") &&
				strcmp((char*)iData[vi].descriptor,"ERF")
				) ;vi++);
		if (iData[vi].descriptor[0])
		{
			voiceData=&iData[vi];
			*synthActif=BABILE_SYNTHACTIVE_COLIBRI;
			*synthAvailable|= BABILE_SYNTHMODULE_COLIBRI;
		}
		/* CHECK for TTF */
		for (vi=0; iData[vi].descriptor[0] && (
				strcmp((char*)iData[vi].descriptor,"IVX")
				) ;vi++);
		if (iData[vi].descriptor[0])
		{
			*nlpModule=BABILE_NLPMODULE_TTF;
		}
		/* CHECK for NLPE */
		for (vi=0; iData[vi].descriptor[0] && (
				strcmp((char*)iData[vi].descriptor,"PHO")
				) ;vi++);
		if (iData[vi].descriptor[0])
		{
			*nlpModule=BABILE_NLPMODULE_NLPE;
		}
		/* CHECK for TEXT */
		for (vi=0; iData[vi].descriptor[0] && (
				strcmp((const char*)iData[vi].descriptor,"TXT")
				) ;vi++);
		if (iData[vi].descriptor[0] && iData[vi].pDbId->type==X_RAM )
		{
			*pText=(const char*)iData[vi].pDbId->link;
		}
		*ppSynthLS = voiceData;
		*ppNLPLs=iData;
	}
	return iData;
}
#endif
#endif


