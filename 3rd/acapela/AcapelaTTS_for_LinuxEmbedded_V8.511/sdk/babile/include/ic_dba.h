/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * $Id: ic_dba.h,v 1.11 2016/03/17 08:48:20 biesie Exp $
 *
 * File name:           $RCSfile: ic_dba.h,v $
 * File description :   DBASPI INTERFACE's CONSTANT Definition
 * Module :             COMMON\DBASPI\
 * File location :      .\common\dbaspi\api\
 * Purpose:             
 * Author:              NM
 * History : 
 *            12/09/2001    Created
 *
 *
 * Copyright (c) 1998-2001 BABEL TECHNOLOGIES
 * All rights reserved.
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN
 * AGREEMENT OR ROYALTY FEES.
 *
 */


#ifndef __DBASPI__IC_DBA_H__
#define __DBASPI__IC_DBA_H__


/**********************************************************************/
/*   DBASPI CONSTANTS DESCRIPTION                                     */
/**********************************************************************/
/* Create the BB_DbMod enum : correspond to the mode in wich the db is opened */

#define   BB_DB_CLOSED 0xFF                /* Not yet opened */
#define   BB_DB_READ   0x01                /* Opened for read operations */
#define   BB_DB_WRITE  0x02                /* Opened for write operations */
#define   BB_DB_READ_WRITE  BB_DB_READ | BB_DB_WRITE   /* Opened for read-write operations */
#define   BB_DbMode    BB_U16

/* Additional flags */
#define BB_DBFLAGS_ENDIAN_MASK	  0x03
#define BB_DBFLAGS_NATIVE		  0
#define BB_DBFLAGS_LE			  1
#define BB_DBFLAGS_BE			  2
#define BB_DBFLAGS_INVERSE		  3

#define BB_DBERRORCODE_FLAGS						-1   /* error codes (in handle.flags) */
#define BB_DBERRORCODE_FILEMAP_LINUX				-2
#define BB_DBERRORCODE_FILEMAP_WIN					-3
#define BB_DBERRORCODE_ABSTRACT						-4
#define BB_DBERRORCODE_ABSTRACT_NOBASE				-5
#define BB_DBERRORCODE_FILEOPEN						-6
#define BB_DBERRORCODE_BBDBID						-7

/* Memory type descriptions */
/* MEMORY SPACE */
#define BB_DBSPACE_READWRITE      BB_BIT0
#define BB_DBSPACE_READONLY       0
/* DATABASE TYPE */
#define BB_DBTYPE_FILE            BB_BIT1
#define BB_DBTYPE_MEM             0
/* DIRECTACCESS ATTRIBUTE */
#define BB_DBACCESS_DIRECT        BB_BIT2
#define BB_DBACCESS_UNDIRECT      0
/* BUFFERIZATION ATTRIBUTE */
#define BB_DBBUFFERIZE_OFF        BB_BIT3
#define BB_DBBUFFERIZE_ON         0

#define BB_DBSIZEPROTECT_ON			BB_BIT4
#define BB_DBSIZEPROTECT_OFF        0

#define BB_DBMEM_MEMORYMAP			BB_BIT9



#define BB_CREATEDBMEMTYPE(memSpace, memType, memAccess, memSpeed)\
  memSpace | memType | memAccess | memSpeed

/* predefined memory types */
#define BB_ROM_DBMEMTYPE      BB_CREATEDBMEMTYPE(BB_DBSPACE_READONLY, BB_DBTYPE_MEM, BB_DBACCESS_DIRECT, BB_DBBUFFERIZE_OFF)
#define BB_RAM_DBMEMTYPE      BB_CREATEDBMEMTYPE(BB_DBSPACE_READWRITE, BB_DBTYPE_MEM, BB_DBACCESS_DIRECT, BB_DBBUFFERIZE_OFF)
#define BB_ROFILE_DBMEMTYPE   BB_CREATEDBMEMTYPE(BB_DBSPACE_READONLY, BB_DBTYPE_FILE, BB_DBACCESS_UNDIRECT, BB_DBBUFFERIZE_ON)
#define BB_RWFILE_DBMEMTYPE   BB_CREATEDBMEMTYPE(BB_DBSPACE_READWRITE, BB_DBTYPE_FILE, BB_DBACCESS_UNDIRECT, BB_DBBUFFERIZE_ON)
#define BB_DEFAULT_DBMEMTYPE  BB_CREATEDBMEMTYPE(BB_DBSPACE_READONLY, BB_DBTYPE_FILE, BB_DBACCESS_UNDIRECT, BB_DBBUFFERIZE_ON)
#define X_RAM_MASK			(BB_BIT1|BB_BIT2|BB_BIT3)
#define X_RAM_MASK_SWAPE		(X_RAM_MASK<<8)
#define X_FILE_MASK			(BB_BIT1)
#define X_RAM				(BB_DBTYPE_MEM|BB_DBACCESS_DIRECT|BB_DBBUFFERIZE_OFF)
#define X_RAM_SWAPE			(X_RAM<<8)
#define X_FILE				(BB_DBTYPE_FILE)
#define X_RAMU				(BB_DBTYPE_MEM|BB_DBACCESS_UNDIRECT|BB_DBBUFFERIZE_OFF)
#define X_RAMU_SWAPE		(X_RAMU<<8)
#define X_ONLY_RAM			(BB_BIT8|X_RAM)
#define X_FILEMAP			(BB_DBMEM_MEMORYMAP|X_RAM)
#define X_PALMSD			(BB_BIT10|X_FILE)
#define	X_PALMDB			(BB_BIT11|X_FILE)
#define	X_PALMDBRAM			(BB_BIT11|X_RAM)
#define	X_PALMDBRAM_SWAPE	((BB_BIT11>>8)|X_RAM_SWAPE)
#define	X_PALMDBRAMU		(BB_BIT11|X_RAMU)
#define	X_PALMDBRAMU_SWAPE	((BB_BIT11>>8)|X_RAMU_SWAPE)
/* --> Done for BabTTS: pointer to function and special handle for using CBabFileAbstract */
#define X_ABSTRACT			(BB_BIT12|X_FILE)


typedef BB_U16 BB_DbMemType;




#endif /* __DBASPI__IC_DBA_H__ */

