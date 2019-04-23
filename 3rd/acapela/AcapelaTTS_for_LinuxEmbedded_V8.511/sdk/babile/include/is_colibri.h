#ifndef __ACAPELA__IS_COLIBRI_H__
#define __ACAPELA__IS_COLIBRI_H__

/*This is an automaticatly genarated file by ../inc/colibri_h_to_is_colibri_h.pl ../inc/colibri.h ../inc/is_colibri.h*/

static const char* const ALF_ReturnCode_Strings[]={
"ALF_NO_ERROR",	/*-0*/
"ALF_EOF",	/*-1*/
"ALF_NO_MEMORY",	/*-2*/
"ALF_NULL_POINTER",	/*-3*/
"ALF_INVALID_DATA",	/*-4*/
"ALF_UNKNOWN_SETTING",	/*-5*/
"ALF_READONLY_SETTING",	/*-6*/
"ALF_NOT_INITIALIZED",	/*-7*/
"ALF_FILE_NOT_FOUND",	/*-8*/
"ALF_BAD_MAGIC",	/*-9*/
"ALF_BAD_VERSION",	/*-10*/
"ALF_NO_PROCESS",	/*-11*/
"",
"",
"",
"",
"",
"",
"",
"",
"",
"ALF_MODEL_INVALID_STATE",	/*-21*/
"ALF_MODEL_TOO_MANY_DYNAMIC_WINDOWS",	/*-22*/
"",
"",
"",
"",
"",
"",
"",
"",
"ALF_LABEL_BAD_FORMAT",	/*-31*/
"ALF_LABEL_PARSE_ERROR",	/*-32*/
"",
"",
"",
"",
"",
"",
"",
"",
"ALF_TREE_NODE_NOT_FOUND",	/*-41*/
"",
"",
"",
"",
"",
"",
"",
"",
"",
"ALF_TAG_CANNOT_ADD",	/*-51*/
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"ALF_INVALID_LICENSE",	/*-71*/
"",
"",
"",
"",
"",
"",
"",
"",
"",
"ALF_QUEUE_WRITE_ERROR",	/*-81*/
"ALF_QUEUE_READ_ERROR",	/*-82*/
"ALF_QUEUE_EMPTY",	/*-83*/
"ALF_QUEUE_FULL",	/*-84*/
"ALF_QUEUE_CANNOT_RESIZE",	/*-85*/
"",
"",
"",
"",
"",
"ALF_ENGINE_NOT_READY",	/*-91*/
"ALF_ENGINE_TOO_MANY_PHONEMES",	/*-92*/
"ALF_ENGINE_UNKNOWN_PHONEME",	/*-93*/
"ALF_ENGINE_PHONEME_TOO_LONG",	/*-94*/
"ALF_ENGINE_FLUSHERROR",	/*-95*/
"",
"",
"",
"",
"",
"ALF_ENGINE_CBREADPHO",	/*-101*/
"ALF_ENGINE_CBWAIT",	/*-102*/
""};
#define ALF_ReturnCode_MAX_VALUE ( sizeof(ALF_ReturnCode_Strings) / sizeof(char*) )
#define ALF_ReturnCode_2str(code)   (((code >= ALF_ReturnCode_MAX_VALUE)||(code<0)||(*ALF_ReturnCode_Strings[abs((int)code)]=='\0'))? "UNDEFINED" : ALF_ReturnCode_Strings[abs((int)code)])

#endif /*__ACAPELA__IS_COLIBRI_H__*/
