/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * $Id: defines.h,v 1.14 2013/12/06 14:20:12 biesie Exp $
 *
 * File name:           $RCSfile: defines.h,v $
 * File description :   BABEL GENERIC C HEADER
 * Module :             COMMON
 * File location :      .\common\def\
 * Purpose:             This header is included into each BABEL project
 *                      It contains the project specific compilation flags
 * Author:              NM
 * History : 
 *            21/06/2001    Created     NM
 *            03/09/2001    Update      NM
 *
 *
 * Copyright (c) 1998-2001 BABEL TECHNOLOGIES
 * All rights reserved.
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN
 * AGREEMENT OR ROYALTY FEES.
 *
 */


#ifndef __COMMON__BB_DEFINES_H__
#define __COMMON__BB_DEFINES_H__

  
/**********************************************************************/
/*   PLATFORM DEFINITION                                              */
/**********************************************************************/
/* PLATFORM definition ("bb_def.h" & "bb_types.h")                    */
/*   - Through predefined platforms (see "bb_def.h" for more info)    */
/*       #define TI_C54X or WIN32                                     */
/*   - or through customized platform (see "bb_types.h" for more info)*/
/*     CPU TYPE (__BB_CPU_TYPE__)                                     */
/*       #define __BB_CPU8__ or __BB_CPU16__ or __BB_CPU32__          */
/*     MEMORY TYPE (__BB_MEMORY_TYPE__)                               */
/*       #define __BB_MEM8__ or __BB_MEM16__ or __BB_MEM32__          */
/**********************************************************************/
/* TO-DO : Write your platform definition here */
/* e.g. :#define TI_C54X */

/**********************************************************************/
/*   BABEL COMMON INCLUDE FILES                                       */
/**********************************************************************/
#include "bb_def.h"     /* BABEL's constants definitions */
#include "bb_types.h"   /* BABEL's types definitions */
#include "bb_mem.h"     /* BABEL's memory definitions */



/**********************************************************************/
/*   PROJECT SPECIFIC COMPILATION FLAGS                               */
/**********************************************************************/
/* TO-DO : Add your specific preprocessor defines file here */ 
/* <my_module_path>\def\<my_specific_path>\<my_defines_file.h> */


#include "defines_generic.h"
#ifndef __BB_DEFINES_DEFINED__
#error "Don't forget to include your preprocessor defines."
#endif

#endif /* __COMMON__BB_DEFINES_H__ */
