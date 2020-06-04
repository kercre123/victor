#ifndef __BBT__IS_BBSEL_H__
#define __BBT__IS_BBSEL_H__



char const * const BBSEL_errorStrings[]={
"BBSEL_ENOERROR",/*			0	*/
"BBSEL_ENOMEM",/*			-1	*/
"BBSEL_ENULLARG",/*			-2	*/
"BBSEL_EINVALIDARG",/*		-3	*/
"BBSEL_ENOTIMPLEMENTED",/*	-4	*/
"BBSEL_ENODATAFILE",/*		-5	*/
"BBSEL_EGENERIC",/*			-6	*/
"BBSEL_ESYSTEM",/*			-7	*/
"BBSEL_EAUDIOFORMAT",/*		-8	*/
"BBSEL_EDATAVERSION",/*		-9	*/
"BBSEL_EPROCESS",/*			-10	*/
"BBSEL_EMAX_PHO",/*			-11	*/
"BBSEL_EDATAOPEN",/*		-12	*/
"BBSEL_EAUDIOBUFFER",/*		-13	*/
"BBSEL_EDATACOMATIBILITY",/*-14	*/
"BBSEL_EPHONEME",/*			-15	*/
"UNKNOWN",	
"UNKNOWN",	
"UNKNOWN",	
"UNKNOWN",	
"BBSEL_ENOINPUT",/*			-20	*/
"BBSEL_EDATAOPEN_DBID", /*-21*/
"BBSEL_EDATAMEMTYPE", /*-22*/
0};

#define BBSEL_MAXERROR_VALUE_STRING (sizeof(BBSEL_errorStrings)/sizeof(char*))

#endif /* __BBT__IS_BBSEL_H__ */
