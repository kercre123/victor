#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

/*
 *----------------------------------------------------------------------------------------------------------------------------------------
 * (c) Copyright 2012, Dialog Semiconductor
 * All Rights Reserved
 *----------------------------------------------------------------------------------------------------------------------------------------
 * DIALOG SEMICONDUCTOR CONFIDENTIAL
 *----------------------------------------------------------------------------------------------------------------------------------------
 * This code includes Confidential, Proprietary Information and is a Trade 
 * Secret of Dialog Semiconductor. All use, disclosure, and/or reproduction 
 * is prohibited unless authorized in writing.
 *----------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Description : IO, data type definitions and memory map selector.
 *
 * Remark(s)   : None.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Synchronicity keywords:
 *
 * Id         : $Id: global_io.h.rca 1.1 Wed Oct 24 09:42:42 2012 wroovers Experimental wroovers $
 * Source     : $Source: /services/syncdata/hazelaar/8500/server_vault/applab/14580/sw/keil/keil_full_emb_SWHL6V16LL6V32_24okt_crosstest_OK_fpga14b/src/inc/global_io.h.rca $
 * 
 *----------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Synchronicity history:
 *
 * Log        : $Log: global_io.h.rca $
 * Log        : 
 * Log        :  Revision: 1.1 Mon Oct  1 11:08:33 2012 wroovers
 * Log        :  First Working set
 * Log        : 
 * Log        :  Revision: 1.2 Mon Mar 19 11:52:13 2012 snijders
 * Log        :  Updated the '*int*' typedef's. Put the 'bool' (C++ only) typedef in comment, see comment.
 * Log        :  Added typedef HWORD and updated the already existing WORD and DWORD (according to the ARM data type definitions).
 * Log        :  Removed the IAR depending '__far' and 'inline' defines (obsolete).
 * Log        :  Removed the CR16C_SW_V4 depending '__far' define (obsolete).
 * Log        :  Removed the 'NULL' define (most probably also obsolete).
 * Log        : 
 * Log        :  Revision: 1.1 Tue Mar  6 11:00:42 2012 snijders
 * Log        :  Initial check in for Project bbtester. Copied from "../sitel_io.h" and renamed to "global_io.h".
 * Log        :  Updated the header, no further changes yet.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 *
 * History:
 *
 *   sc14580a
 *   --------
 *   19-mar-2012/HS: Updated the '*int*' typedef's. Put the 'bool' (C++ only) typedef in comment, see comment.
 *                   Added typedef HWORD and updated the already existing WORD and DWORD (according to the ARM data type definitions).
 *                   Removed the IAR depending '__far' and 'inline' defines (obsolete).
 *                   Removed the CR16C_SW_V4 depending '__far' define (obsolete).
 *                   Removed the 'NULL' define (most probably also obsolete).
 *   06-mar-2012/HS: Initial check in for Project bbtester. Copied from "../sitel_io.h" and renamed to "global_io.h".
 *                   Updated the header, no further changes yet.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
*/


typedef unsigned char      uint8;   //  8 bits
typedef char               int8;    //  8 bits
typedef unsigned short     uint16;  // 16 bits
typedef short              int16;   // 16 bits
typedef unsigned long      uint32;  // 32 bits
typedef long               int32;   // 32 bits
typedef unsigned long long uint64;  // 64 bits
typedef long long          int64;   // 64 bits


/* See also "Variable types supported" on pag. 307 of the (Doulos) Cortex-M0 / SoC 1.0 training documentation,
   the compiler already supports the basic type 'bool' (8-bit Boolean for C++ only).
#ifndef defineCPP
typedef unsigned char      bool;
#endif
*/


/* See also "Data Types" on pag. 21 of the (Doulos) Cortex-M0 / SoC 1.0 training documentation. */
typedef unsigned char      BYTE;     //  8 bits = Byte
typedef unsigned short     HWORD;    // 16 bits = Halfword
typedef unsigned long      WORD;     // 32 bits = Word
typedef long long          DWORD;    // 64 bits = Doubleword


#include "datasheet.h"

#endif
