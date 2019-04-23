/* $Id: is_nlpe.h,v 1.14 2015/05/06 08:34:14 biesie Exp $ */

#ifndef __BB_NLPE__IS_NLPE_H__
#define __BB_NLPE__IS_NLPE_H__

#ifdef __cplusplus
extern "C"
{
#endif

	static const char* const NLPE_errorString[] = { "E_NLPE_NOERROR", /* 0 */
											"E_NLPE_NOMEM", /* -1 */
											"E_NLPE_MEMFREE", /* -2 */
											"E_NLPE_ISPLAYING", /* -3 */
											"E_NLPE_SPEAKERROR", /* -4 */
											"E_NLPE_WAVEOUTWRITE", /* -5 */
											"E_NLPE_WAVEOUTNOTFREE", /* -6 */
											"E_NLPE_PROCESSERROR", /* -7 */
											"E_NLPE_REGISTRYERROR", /* -8 */
											"E_NLPE_NOREGISTRY", /* -9 */
											"E_NLPE_NOTVALIDPARAMETER", /* -10 */
											"E_NLPE_THREADERROR", /* -11 */
											"E_NLPE_NONLP", /* -12 */
											"E_NLPE_INVALIDTAG", /* -13 */
											"E_NLPE_FILEWRITE", /* -14 */
											"E_NLPE_FILEOPEN", /* -15 */
											"E_NLPE_BADPHO", /* -16 */
											"E_NLPE_DICT_OPEN", /* -17 */
											"E_NLPE_DICT_WRITE", /* -18 */
											"E_NLPE_DICT_READ", /* -19 */
											"E_NLPE_DICT_NOENTRY", /* -20 */
											"E_NLPE_NOTIMPLEMENTED", /* -21 */
											"E_NLPE_NODBA", /* -22 */
											"E_NLPE_NODICT", /* -23 */
											"E_NLPE_NOTVALIDLICENSE", /* -24*/
											"E_NLPE_INITBNF", /* -25*/
											"E_NLPE_NODBABNF", /* -26*/
											"E_NLPE_INITPRP", /* -27*/
											"E_NLPE_NODBAPRP", /* -28*/
											"E_NLPE_INITUDIC", /* -29*/
											"E_NLPE_INITDIC", /* -30*/
											"E_NLPE_INITF0", /* -31*/
											"E_NLPE_NODBAF0", /* -32*/
											"E_NLPE_INITOSO", /* -33*/
											"E_NLPE_NODBAOSO", /* -34*/
											"E_NLPE_INITPHOVECT", /* -35*/
											"E_NLPE_INITTREE", /* -36*/
											"E_NLPE_NODBATREE", /* -37*/
											"E_NLPE_NULLARG", /* -38*/
											"E_NLPE_OUTBUF", /* -39*/
											"E_NLPE_NLPMAP", /* -40*/
											"E_NLPE_PAUTREE", /* -41*/
											"E_NLPE_INITCMA", /* -42*/
											"E_NLPE_INITCMD", /* -43*/
											"UNKNOWN", /* -44*/
											"UNKNOWN", /* -45*/
											"UNKNOWN", /* -46 */
											"UNKNOWN", /* -47 */
											"UNKNOWN", /* -48 */
											"UNKNOWN", /* -49 */
											"E_NLPE_DLSTTS", /* -50 */
											"E_NLPE_NODBAPST" /* -51 */ ,
											"E_NLPE_INITPST" /* -52 */, 
											"E_NLPE_NODBAGRAMVOC"			/*-53*/,
											"E_NLPE_INITGRAMVOC"			/*-54*/,
											"E_NLPE_NODBAGRAMOOV"			/*-55*/,
											"E_NLPE_INITGRAMOOV"			/*-56*/,
											"E_NLPE_INITGRAM" 			/*-57*/,
											"E_NLPE_NODBATONE"			/*-58*/,
											"E_NLPE_INITTONE" 			/*-59*/,	
											"E_NLPE_AOPARSER_DATA"		/*-60*/,
											"E_NLPE_VERSION_OSO",	/*-61*/	
											"E_NLPE_NODBADIAC_ART",	/*-62*/
											"E_NLPE_NODBADIAC_ARQ",	/*-63*/
											"E_NLPE_INITDIAC_ART",	/*-64*/
											"E_NLPE_INITDIAC_ARQ",	/*-65*/
											"UNKNOWN",	
											"UNKNOWN",	
											"UNKNOWN",	
											"UNKNOWN",	
											"E_DICTM_NOTVALIDPARAMETER",	/*-70*/
											"E_DICTM_DICT_EXIST",
											"E_DICTM_DICT_WRITE",
											"E_DICTM_DICT_NOENTRY",
											"E_DICTM_PROCESSERROR",
											"E_DICTM_DICT_BADPHO",	/*-75*/
											"E_DICTM_INVALID_BINARY",
											"E_DICTM_DICT_NOOVERWRITEENTRY",
											"E_DICTM_WRONG_VERSION",		/*-78*/
											"UNKNOWN",	
											"E_NLPE_NLPLAYERSGENERIC", 		/*-80*/
											"E_NLPE_NLPLAYERSTXT", 		/*-81*/
											"E_NLPE_NLPLAYERSPREPRO1", 		/*-82*/
											"E_NLPE_NLPLAYERSPREPRO2", 		/*-83*/
											"E_NLPE_NLPLAYERSWIN", 		/*-84*/
											"E_NLPE_NLPLAYERSWORD", 		/*-85*/
											"E_NLPE_NLPLAYERSMORPH", 		/*-86*/
											"E_NLPE_NLPLAYERSGRA", 		/*-87*/
											"E_NLPE_NLPLAYERSPHN", 		/*-88*/
											"E_NLPE_NLPLAYERSCHUNK", 		/*-89*/
											"E_NLPE_NLPLAYERSPAU", 		/*-90*/
											"E_NLPE_NLPLAYERSPHN2", 		/*-91*/
											"E_NLPE_NLPLAYERSSYL", 		/*-92*/
											"E_NLPE_NLPLAYERSTON", 		/*-93*/
											"E_NLPE_NLPLAYERSDUR", 		/*-94*/
											"E_NLPE_NLPLAYERSF0", 		/*-95*/
											"E_NLPE_NLPLAYERSREWRITE", 		/*-96*/
											"E_NLPE_NLPLAYERSEND", 		/*-97*/
											"E_NLPE_NLPLAYERSTERMINATED" 		/*-98*/
	};

#define NLPE_MAX_ERROR_VALUE ( sizeof(NLPE_errorString) / sizeof(BB_TCHAR*) )

#define NLPE_error2str(code)   (((code <= -(signed)NLPE_MAX_ERROR_VALUE)||(code>0))? "UNDEFINED" : NLPE_errorString[-code])

#ifdef __cplusplus
}
#endif

#endif /* __BB_NLPE__IS_NLPE_H__ */
