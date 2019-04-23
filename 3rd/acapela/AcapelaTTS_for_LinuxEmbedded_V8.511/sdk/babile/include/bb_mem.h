/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * $Id: bb_mem.h,v 1.7 2006/04/19 08:31:49 biesie Exp $
 *
 * File name:           $RCSfile: bb_mem.h,v $
 * File description :   BABEL GENERIC MEMORY DEFINITION
 * Module :             COMMON
 * File location :      .\common\inc\
 * Purpose:             This header is included into each BABEL project
 *                      through "defines.h"
 *                      It contains the BABEL memory definition
 * Author:              PB
 * History : 
 *            30/08/2001    Created     PB
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


#ifndef __COMMON__BB_MEM_H__
#define __COMMON__BB_MEM_H__

/**********************************************************************/
/*   MEMORY CONSTANTS DESCRIPTION                                     */
/**********************************************************************/
/* Memory Attribute object */
#define     BB_MEM_SCRATCH	0           /* scratch memory */
#define     BB_MEM_PERSIST	1           /* persistent memory */
#define		BB_MEM_WRITEONCE 2         /* write-once persistent memory */
#define BB_MemAttrs BB_U8

/* Memory space object */
 /* inspired from TI XDAIS */
#define     BB_IALG_DARAM0  0        /* dual access on-chip data memory */
#define     BB_IALG_DARAM1  1        /* block 1, if independant blocks required */

#define     BB_IALG_SARAM   2        /* single access on-chip data memory */
#define     BB_IALG_SARAM0  2        /* block 0, equivalent to IALG_SARAM */
#define     BB_IALG_SARAM1  3        /* block 1, if independant blocks required */

#define     BB_IALG_DARAM2  4        /* block 2, if a 3rd independent block required */
#define     BB_IALG_SARAM2  5         /* block 2, if a 3rd independent block required */

#define		BB_IALG_NONE	255      /* It is a dummy block -> Don't need to allocate it! */

#define     BB_ANY 50             /* any kind of memory */  /*Not XDAS compliant*/
		/* other types will be defined later */
#define  BB_MemSpace BB_U8


/**********************************************************************/
/*   MEMORY OBJECTS DESCRIPTION                                       */
/**********************************************************************/
/* MEMORY RECORD object */
#ifdef PALMOS5
typedef struct BB_MemRec_Tag {
    BB_U32        size;       /* size in MAU of allocation */
    void          *base;      /* base address of allocated buf */
	BB_U32			id;	  /* requiring module ID */
    BB_U8         alignment;  /* alignment requirement (MAU) */
    BB_MemSpace   space;      /* allocation space */
    BB_MemAttrs   attrs;      /* memory attributes */
} BB_MemRec;
#else
typedef struct BB_MemRec_Tag {
    BB_U32        size;       /* size in MAU of allocation */
    BB_U8         alignment;  /* alignment requirement (MAU) */
    BB_MemSpace   space;      /* allocation space */
    BB_MemAttrs   attrs;      /* memory attributes */
    void          *base;      /* base address of allocated buf */
	BB_U32			id;	  /* requiring module ID */
} BB_MemRec;
#endif

#endif /* __COMMON__BB_MEM_H__ */
