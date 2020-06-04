static void BABILEDEMO_changeInitConfig(BABILE_MemParam* initializationParameters, const char* const loadconfig)
{
	const char * const * c;
	const char * x=NULL;
	int i, trigger=0;
	c=BABILE_sMBRConfigStrings;
	for (i=0;c[i]!=NULL && (*(c[i])==0 || !strstr(loadconfig,c[i]));i++) {}
	if (c[i]) initializationParameters->sMBRConfig=i;
	c=BABILE_sNLPConfigStrings;
	for (i=0;c[i]!=NULL && (*(c[i])==0 || !strstr(loadconfig,c[i]));i++) {}
	if (c[i]) initializationParameters->sNLPConfig=i;
	c=BABILE_sSELConfigStrings;
	i=0;
	do
	{
		for (;c[i]!=NULL && (*(c[i])==0 || !(x=strstr(loadconfig,c[i])));i++) {}
		if (c[i] && x) 
		{
			int v;
			if (trigger==0) initializationParameters->sSELConfig=0;
			trigger=1;
			initializationParameters->sSELConfig|=BABILE_sSELConfigValues[i];
			x+=strlen(c[i]);
			if (*x && (v=atoi(x))>0) initializationParameters->sSELConfig|=v;
			i++;
		}
	}
	while (c[i]);
}

void ChangeSetting(BABILE_Obj* bab,BB_S16 param,char* av,FILE* FILEOUT)
{
	BB_SPTR val;
	if (0>BABILE_getSetting(bab, param, &val))
	{
		testError(bab,NULL,FILEOUT);
	}
	else
	{
		if (param<sizeof(BABILE_settingStrings)/sizeof(char*)) fprintf(FILEOUT,"%s " BB_S32_PFMT " ",BABILE_settingStrings[param],val);
		val=atoi(av);
		if (0>BABILE_setSetting(bab,param,val))
		{
			testError(bab,NULL,FILEOUT);
		}
		else
		{
			fprintf(FILEOUT," --> %ld \n",val);
		}
	}
}

static void BABILEDEMO_changeSettings(FILE* FILEOUT,void* bab, int argpos, int argc, char** argv)
{
	while (argpos<argc-1)				/* Changing parameters */
	{
		if (strcmp(argv[argpos],"VOICECTRL")==0)
		{
			BB_U16 ctrl[128];
			BB_U16 nb_ctrl=1;
			
			int pos=0;
			char* str= argv[argpos+1];
			
			int val;
 			while (sscanf(str, "%i%n", &val, &pos) == 1)
			{
				ctrl[nb_ctrl++]= (BB_U16)val;
				str+= pos;
			}
			ctrl[0]= nb_ctrl-1;
			BABILE_setSetting(bab,BABIL_PARM_SEL_VOICECTRL, (BB_S32) ctrl[0]);
		}
		else
		{
			short j=0;
			while (BABILE_settingStrings[++j])
			{
				if (!strcmp(argv[argpos],BABILE_settingStrings[j]))
				{
					ChangeSetting(bab,j,argv[argpos+1],FILEOUT);
				}
			}
			if (BABILE_settingStrings[j]==NULL && !strcmp(argv[argpos],"DEFAULT"))
			{
				if (0>BABILE_setDefaultParams(bab))
				{
					testError(bab,NULL,FILEOUT);
				}
				else
				{
					fprintf(FILEOUT,"DEFAULT VALUES RESTORED\n");
				}
			}
		}
		argpos+= 2;
	}
}
