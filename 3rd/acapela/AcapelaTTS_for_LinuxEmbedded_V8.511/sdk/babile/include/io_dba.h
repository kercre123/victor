/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * $Id: io_dba.h,v 1.30 2016/03/17 08:48:20 biesie Exp $
 *
 * File name:           $RCSfile: io_dba.h,v $
 * File description :   DBASPI INTERFACE's OBJECTS Definition
 * Module :             COMMON\DBASPI\
 * File location :      .\common\dbaspi\api\
 * Purpose:             
 * Author:              NM
 * History : 
 *            28/08/2001    Created
 *
 *
 * Copyright (c) 1998-2001 BABEL TECHNOLOGIES
 * All rights reserved.
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN
 * AGREEMENT OR ROYALTY FEES.
 *
 */


#ifndef __DBASPI__IO_DBA_H__
#define __DBASPI__IO_DBA_H__


/**********************************************************************/
/*   DBASPI OBJECTS DESCRIPTION                                       */
/**********************************************************************/

#ifndef __DBASPI__PO_DBA_H__

/* BB_DbHndl Object */
typedef struct BB_DbHndlTag
{
/* IMPLEMENTATION SPECIFIC : START */
#ifdef PALMOS5
  const void *call68kFuncP;
  const void *emulStateP;
  void      **bb_dba_fliste;

  
	BB_U32 nbHandleLock;
	BB_U8 **memPtr;

#ifdef PALMOS68K		
	MemHandle *HandleLocked;
#else  
	void      *HandleLocked;
#endif
	void* palmResource;
#endif	
	void* base;
	BB_DbOffsetPtr current;
	BB_DbMemType type;
	BB_DbMemType flags;  /* or error code: cast as BB_S16 */
	BB_U32 size;
#ifdef BBDBASPI_WRITE_ENABLE
	BB_U32 MemAllocated;
#endif
#if (defined( WIN32) || defined( _WIN32) || defined (_WIN32_WCE))  && !defined (__SYMBIAN32__)
	void* memhndl;
	void* filehndl;
#elif (defined(PLATFORM_LINUX) || defined (__linux__))  && !defined (__SYMBIAN32__)
	void* filehndl;
	BB_S32 length;
#elif defined(PALMOS5)
	BB_U16 volRefNum;
	BB_U16 dummy1;
#elif defined(__SYMBIAN32__0)
	void* pFs;
#elif defined(__APPLE__)
#else
	#error /* define a PLATFORM ABOVE */
#endif

/* IMPLEMENTATION SPECIFIC : END */
} BB_DbHndl;


#endif /* __DBASPI__PO_DBA_H__ */


/* BB_DbId Object */
typedef struct BB_DbIdTag
{
/* IMPLEMENTATION SPECIFIC : START */
  BB_TCHAR* link;
  BB_DbHndl handle;
  BB_DbMemType type;
  BB_DbMemType flags;
  BB_U32 size;
/* IMPLEMENTATION SPECIFIC : END */
} BB_DbId;

/* BB_DbLs Object */
struct BB_DbLsTag
{
  BB_TCHAR descriptor[4];
  BB_DbId *pDbId;
};

typedef struct BB_DbLsTag BB_DbLs;




#endif /* __DBASPI__IO_DBA_H__ */
