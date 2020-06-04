#ifndef __BBT__IS_MBRE_H__
#define __BBT__IS_MBRE_H__



char const * const MBRE_errorStrings[]={
"MBR_NO_ERROR",/*                    0	*/
"ERROR_MEMORYOUT",/*                -1	*/
"ERROR_UNKNOWNCOMMAND",/*           -2	*/
"ERROR_SYNTAXERROR",/*              -3	*/
"ERROR_COMMANDLINE",/*              -4	*/
"ERROR_OUTFILE",/*                  -5	*/
"ERROR_RENAMING",/*                 -6	*/
"ERROR_NEXTDIPHONE",/*              -7	*/
"ERROR_NULLARG",/*                  -8	*/
"ERROR_VALUEOUTOFBOUNDS",/*         -9	*/
"ERROR_PRGWRONGVERSION",/*         -10	*/
"UNKNOWN",/*	1	*/		
"UNKNOWN",/*	2	*/
"UNKNOWN",/*	3	*/
"UNKNOWN",/*	4	*/
"UNKNOWN",/*	5	*/
"UNKNOWN",/*	6	*/
"UNKNOWN",/*	7	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"ERROR_TOOMANYPITCH",/*            -20	*/
"ERROR_TOOMANYPHOWOPITCH",/*       -21	*/
"ERROR_PITCHTOOHIGH",/*            -22	*/
"UNKNOWN",/*	3	*/
"UNKNOWN",/*	4	*/
"UNKNOWN",/*	5	*/
"UNKNOWN",/*	6	*/
"UNKNOWN",/*	7	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"ERROR_PHOLENGTH",/*               -30	*/
"ERROR_PHOREADING",/*              -31	*/
"UNKNOWN",/*	2	*/
"UNKNOWN",/*	3	*/
"UNKNOWN",/*	4	*/
"UNKNOWN",/*	5	*/
"UNKNOWN",/*	6	*/
"UNKNOWN",/*	7	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"ERROR_DBNOTFOUND",/*              -40	*/
"ERROR_DBWRONGVERSION",/*          -41	*/
"ERROR_DBWRONGARCHITECTURE",/*     -42	*/
"ERROR_DBNOSILENCE",/*             -43	*/
"ERROR_INFOSTRING",/*              -44	*/
"ERROR_DBCODING",/*                -45	*/
"ERROR_EXTRADBCODING",/*           -46	*/
"ERROR_DBTOOMANYPHO",/*            -47	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"UNKNOWN",/*						-50	*/		
"UNKNOWN",/*	1	*/		
"UNKNOWN",/*	2	*/
"UNKNOWN",/*	3	*/
"UNKNOWN",/*	4	*/
"UNKNOWN",/*	5	*/
"UNKNOWN",/*	6	*/
"UNKNOWN",/*	7	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"ERROR_BINNUMBERFORMAT",/*         -60	*/
"ERROR_PERIODTOOLONG",/*           -61	*/
"ERROR_SMOOTHING",/*               -62	*/
"ERROR_UNKNOWNSEGMENT",/*          -63	*/
"ERROR_CANTDUPLICATESEGMENT",/*    -64	*/
"UNKNOWN",/*	5	*/
"UNKNOWN",/*	6	*/
"UNKNOWN",/*	7	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"ERROR_BOOK",/*                    -70	*/
"ERROR_CODE",/*                    -71	*/
"UNKNOWN",/*	2	*/
"UNKNOWN",/*	3	*/
"UNKNOWN",/*	4	*/
"UNKNOWN",/*	5	*/
"UNKNOWN",/*	6	*/
"UNKNOWN",/*	7	*/
"UNKNOWN",/*	8	*/
"UNKNOWN",/*	9	*/
"WARNING_UPGRADE",/*               -80	*/
"WARNING_SATURATION",/*            -81	*/
};

#define MBRE_MAXERROR_VALUE_STRING (sizeof(MBRE_errorStrings)/sizeof(char*))
#endif  /* __BBT__IS_MBRE_H__ */
