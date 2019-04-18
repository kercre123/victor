#ifndef __BBT__IS_BBNLP_H__
#define __BBT__IS_BBNLP_H__



char const * const BBNLP_errorStrings[]={
"BBNLP_NOERROR", /*							0	*/
"BBNLP_ERROR_NOMEMORY", /*					-1	*/
"BBNLP_ERROR_NODYNMEM", /*					-2	*/
"BBNLP_ERROR_NOTIMPLEMENTED", /*			-3	*/
"BBNLP_ERROR_BADTEXTPROCESSINGOBJECT", /*	-4	*/
"BBNLP_ERROR_NULLPARAMETER", /*				-5	*/
"BBNLP_ERROR_BADPARAMETER", /*				-6	*/
"BBNLP_ERROR_INPUTHANDLE", /*				-7	*/
"BBNLP_ERROR_OUTPUTHANDLE", /*				-8	*/
"BBNLP_ERROR_BINARYERROR", /*				-9	*/
"BBNLP_ERROR_MODULE", /*					-10	*/
"BBNLP_ERROR_DCTCALLBACK", /*				-11	*/
"E_NLPE_NONLP", /*						-12	*/
"E_NLPE_INVALIDTAG", /*						-13	*/
"E_NLPE_FILEWRITE", /*						-14	*/
"E_NLPE_FILEOPEN", /*						-15	*/
"E_NLPE_BADPHO", /*						-16	*/
"E_DICTM_DICT_OPEN", /*						-17	*/
"E_NLPE_DICT_WRITE", /*						-18	*/
"E_DICTM_DICT_READ", /*						-19	*/
"E_NLPE_DICT_NOENTRY", /*					-20	*/
"E_NLPE_NOTIMPLEMENTED / ERROR_UNKNOWN_TYPE", /*-21	*/
"E_NLPE_NODBA", /*							-22	*/
"E_DICTM_NODICT", /*						-23	*/
"ERROR_TTF_NOT_READY", /*					-24	*/
"E_NLPE_INITBNF / ERROR_TTF_TEXTPROCOBJ_NOT_NULL", /*-25	*/
"E_NLPE_NODBABNF / ERROR_PHMAP_INIT", /*	-26	*/	
"E_NLPE_INITPRP / ERROR_PHMAP_APPLY", /*	-27	*/
"E_NLPE_NODBAPRP / ERROR_PHOLISTS_INIT", /*	-28	*/
"E_NLPE_INITUDIC / ERROR_SENTENCE_TOO_LONG", /*-29	*/
"E_NLPE_INITDIC", /*						-30	*/
"E_NLPE_INITF0", /*							-31	*/
"E_NLPE_NODBAF0", /*						-32	*/
"E_NLPE_INITOSO", /*						-33	*/
"E_NLPE_NODBAOSO", /*						-34	*/
"E_NLPE_INITPHOVECT", /*					-35	*/
"E_NLPE_INITTREE", /*						-36	*/
"E_NLPE_NODBATREE", /*						-37	*/
"E_NLPE_NULLARG", /*						-38	*/
"E_NLPE_OUTBUF", /*						-39	*/
"E_NLPE_NLPGENEREF0", /*					-40	*/
"E_NLPE_NLPDURATION", /*					-41	*/
"E_NLPE_NLPTONE", /*						-42	*/
"E_NLPE_NLPWORDACCENT", /*					-43	*/
"E_NLPE_NLPINSERTPAUSE", /*					-44	*/
"E_NLPE_NLPSYLLABER", /*					-45	*/
"E_NLPE_NLPPHONEM", /*						-46	*/
"E_NLPE_NLPTRANSCRIPTION", /*				-47	*/
"E_NLPE_NLPWORD", /*						-48	*/
"E_NLPE_NLPWORDIN", /*						-49	*/
"E_NLPE_DLSTTS", /*							-50	*/
"E_NLPE_NODBAPST", /*						-51	*/
"E_NLPE_INITPST", /*						-52	*/
"E_NLPE_NODBAGRAMVOC", /*						-53	*/
"E_NLPE_INITGRAMVOC", /*						-54	*/
"E_NLPE_NODBAGRAMOOV", /*						-55	*/
"E_NLPE_INITGRAMOOV", /*						-56	*/
"E_NLPE_INITGRAM", /*						-57	*/
"E_NLPE_NODBATONE", /*						-58	*/
"E_NLPE_INITTONE", /*						-59	*/
"E_NLPE_AOPARSER_DATA", /*					-60	*/
"BBNLP_ERROR_UNDEF", /*						-61	*/
"BBNLP_ERROR_UNDEF", /*						-62	*/
"BBNLP_ERROR_UNDEF", /*						-63	*/
"BBNLP_ERROR_UNDEF", /*						-64	*/
"BBNLP_ERROR_UNDEF", /*						-65	*/
"BBNLP_ERROR_UNDEF", /*						-66	*/
"BBNLP_ERROR_UNDEF", /*						-67	*/
"BBNLP_ERROR_UNDEF", /*						-68	*/
"BBNLP_ERROR_UNDEF", /*						-69	*/
"E_DICTM_NOTVALIDPARAMETER", /*						-70	*/
"E_DICTM_DICT_EXIST", /*						-71	*/
"E_DICTM_DICT_WRITE", /*						-72	*/
"E_DICTM_DICT_NOENTRY", /*						-73	*/
"E_DICTM_PROCESSERROR", /*						-74	*/
"E_DICTM_DICT_BADPHO", /*						-75	*/
"E_DICTM_INVALID_BINARY", /*						-76	*/
"E_DICTM_DICT_NOOVERWRITEENTRY", /*						-77	*/
"E_DICTM_WRONG_VERSION", /*						-78	*/
"BBNLP_ERROR_UNDEF", /*						-79	*/
"BBNLP_ERROR_UNDEF", /*						-80	*/
"BBNLP_ERROR_UNDEF", /*						-81	*/
"BBNLP_ERROR_UNDEF", /*						-82	*/
"BBNLP_ERROR_UNDEF", /*						-83	*/
"BBNLP_ERROR_UNDEF", /*						-84	*/
"BBNLP_ERROR_UNDEF", /*						-85	*/
"BBNLP_ERROR_UNDEF", /*						-86	*/
"BBNLP_ERROR_UNDEF", /*						-87	*/
"BBNLP_ERROR_UNDEF", /*						-88	*/
"BBNLP_ERROR_UNDEF", /*						-89	*/
"BBNLP_ERROR_UNDEF", /*						-90	*/
"BBNLP_ERROR_UNDEF", /*						-91	*/
"BBNLP_ERROR_UNDEF", /*						-92	*/
"BBNLP_ERROR_UNDEF", /*						-93	*/
"BBNLP_ERROR_UNDEF", /*						-94	*/
"BBNLP_ERROR_UNDEF", /*						-95	*/
"BBNLP_ERROR_UNDEF", /*						-96	*/
"BBNLP_ERROR_UNDEF", /*						-97	*/
"BBNLP_ERROR_UNDEF", /*						-98	*/
"BBNLP_ERROR_UNDEF", /*						-99	*/
"IVX_TTF_ERROR",	/*						-100 */
"IVX_TTF_PHBUFFER_NO_SPACE",/*				-101 */	
"IVX_TTF_NOT_IMPLEMENTED",/*				-102 */
"IVX_TTF_INVALID_PHONEME",/*				-103 */	
"IVX_TTF_PHBUFFER_NO_INFO_SAVED",/*			-104 */
"IVX_TTF_PHBUFFER_NO_PHO_SAVED",/*			-105 */
"IVX_TTF_ILLEGAL_NUMBER_OF_ELEMENTS",/*		-106 */
0
};
#define BBNLP_MAX_ERROR_VALUE				106

#endif /*__BBT__IS_BBNLP_H__*/
