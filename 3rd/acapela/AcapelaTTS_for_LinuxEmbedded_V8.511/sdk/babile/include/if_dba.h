/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * $Id: if_dba.h,v 1.22 2016/03/17 08:48:20 biesie Exp $
 *
 * File name:           $RCSfile: if_dba.h,v $
 * File description :   DBASPI INTERFACE's FUNCTIONS Definition
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


#ifndef __DBASPI__IF_DBA_H__
#define __DBASPI__IF_DBA_H__


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#ifdef BBDEBUG_SPI
#define BBDEBUG_FILE    extern void* _BABILE_init_log;
int BBDEBUG_FOPEN(void** X,BB_TCHAR* A, BB_TCHAR* B)  ;
int BBDEBUG_FPRINT1(void* X, BB_TCHAR *A)  ;
int BBDEBUG_FPRINT2(void* X, BB_TCHAR *A, void* B)  ;
int BBDEBUG_FCLOSE(void* X)  ;
#else
#define BBDEBUG_FOPEN(X,A,B)
#define BBDEBUG_FPRINT1(X,A)
#define BBDEBUG_FPRINT2(X,A,B)
#define BBDEBUG_FCLOSE(X)
#define BBDEBUG_FILE 
#endif
/**********************************************************************/
/*   DBASPI FUNCTIONS PROTOTYPES (extern)                             */
/**********************************************************************/

/* Open/Close Functions */
BB_DbHndl *BB_dbOpen(BB_DbId *dbId, BB_DbMode mode);

BB_ERROR BB_dbGetError(BB_DbId *dbId);

BB_ERROR BB_dbClose(BB_DbHndl *dbHandle);

/* Tell Functions */
BB_DbOffset32 BB_dbTell( BB_DbHndl *dbHandle);
/* Get RAM Pointer */
extern void* BB_dbTellRAM(BB_DbHndl *dbHandle);
/* Get Memory Type Information */
BB_DbMemType BB_dbGetMemType(BB_DbHndl *dbHandle);

extern BB_ERROR BB_dbFeof(BB_DbHndl* dbHandle); /* Only accurate for type FILE */

/* Seek Functions */
#ifdef PALMOS5
extern BB_ERROR BB_dbDebugSet(BB_DbHndl *dbHandle, const BB_S32 s32offset);
#else
#define BB_dbDebugSet(X,Y)
#endif
BB_ERROR BB_dbSeekSet(BB_DbHndl *dbHandle, const BB_DbOffset32 dbOffset);
extern BB_ERROR BB_dbSeekCurrent(BB_DbHndl *dbHandle, const BB_DbOffset32 dbOffset);
extern BB_ERROR BB_dbSeekEnd(BB_DbHndl *dbHandle, const BB_DbOffset32 dbOffset);

/* Single Read Functions */
extern BB_U8 BB_dbReadU8(BB_DbHndl *dbHandle);
#define BB_dbReadS8(dbHandle) (BB_S8)BB_dbReadU8(dbHandle)
#ifdef PALMOS5
extern BB_TCHAR BB_dbReadTCHAR(BB_DbHndl *dbHandle);
#else
extern int BB_dbReadTCHAR(BB_DbHndl *dbHandle);
#endif
extern BB_U16 BB_dbReadU16(BB_DbHndl *dbHandle);
#define BB_dbReadS16(dbHandle) (BB_S16)BB_dbReadU16(dbHandle)
extern BB_U32 BB_dbReadU32(BB_DbHndl *dbHandle);
#define BB_dbReadS32(dbHandle) (BB_S32)BB_dbReadU32(dbHandle)
#define BB_dbReadOffset(dbHandle) (BB_S32)BB_dbReadU32(dbHandle)

/* Multiple Read Functions */
extern BB_ERROR BB_dbReadMultiU8(BB_DbHndl *dbHandle, BB_U8 *pu8value, const BB_U32 u32length);
#define BB_dbReadMultiS8(dbHandle, pvalue, length) BB_dbReadMultiU8(dbHandle, (BB_U8*)pvalue, length)
extern BB_ERROR BB_dbReadMultiU16(BB_DbHndl *dbHandle, BB_U16 *pu16value, const BB_U32 u32length);
#define BB_dbReadMultiS16(dbHandle, pvalue, length) BB_dbReadMultiU16(dbHandle, (BB_U16*)pvalue, length)
extern BB_ERROR BB_dbReadMultiU32(BB_DbHndl *dbHandle, BB_U32 *pu32value, const BB_U32 u32length);
#define BB_dbReadMultiS32(dbHandle, pvalue, length) BB_dbReadMultiU32(dbHandle, (BB_U32*)pvalue, length)
#define BB_dbReadMultiTCHAR(dbHandle, pvalue, length) (BB_TCHAR)BB_dbReadMultiU8(dbHandle, (BB_U8*)pvalue, length)

extern BB_NU32 BB_dbRead(void* buffer, BB_S32 size, BB_S32 count, BB_DbHndl* dbHandle);


/* Single Write Functions */
extern BB_DbOffset32 BB_dbWriteU8(BB_DbHndl *dbHandle, const BB_U8 u8value);
#define BB_dbWriteS8(dbHandle, value) BB_dbWriteU8(dbHandle, (BB_S8)value)
extern BB_DbOffset32 BB_dbWriteU16(BB_DbHndl *dbHandle, const BB_U16 u16value);
#define BB_dbWriteS16(dbHandle, value) BB_dbWriteU16(dbHandle, (BB_S16)value)
extern BB_DbOffset32 BB_dbWriteU32(BB_DbHndl *dbHandle, const BB_U32 u32value);
#define BB_dbWriteS32(dbHandle, value) BB_dbWriteU32(dbHandle, (BB_S32)value)

/* Multiple Write Functions */
extern BB_DbOffset32 BB_dbWriteMultiU8(BB_DbHndl *dbHandle, BB_U8 *pu8value, const BB_S32 s32length);
#define BB_dbWriteMultiS8(dbHandle, pvalue, length) BB_dbWriteMultiU8(dbHandle, (BB_S8)pvalue, length)
extern BB_DbOffset32 BB_dbWriteMultiU16(BB_DbHndl *dbHandle, BB_U16 *pu16value, const BB_S32 s32length);
#define BB_dbWriteMultiS16(dbHandle, pvalue, length) BB_dbWriteMultiU16(dbHandle, (BB_S16)pvalue, length)
extern BB_DbOffset32 BB_dbWriteMultiU32(BB_DbHndl *dbHandle, BB_U32 *pu32value, const BB_S32 s32length);
#define BB_dbWriteMultiS32(dbHandle, pvalue, length) BB_dbWriteMultiU32(dbHandle, (BB_S32)pvalue, length)
#define BB_dbWriteMultiTCHAR(dbHandle, pvalue, length) (BB_DbOffset)BB_dbWriteMultiU8(dbHandle, (BB_U8*)pvalue, length)

extern BB_S32 BB_dbDummyCall(BB_U16 a,BB_U16 b,BB_U32 c );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DBASPI__IF_DBA_H__ */
