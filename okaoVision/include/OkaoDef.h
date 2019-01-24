/*-------------------------------------------------------------------*/
/*  Copyright(C) 2003-2012 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
#ifndef OKAODEF_H__
#define OKAODEF_H__

#if (defined( WIN32 ) || defined( WIN64 ))
#include <windows.h>
#else   /* WIN32 || WIN64 */
#include <stdlib.h>
#endif  /* WIN32 || WIN64 */


typedef     signed char         INT8;       /*  8 bit Signed   Integer  */
typedef     unsigned char       UINT8;      /*  8 bit Unsigned Integer  */
typedef     signed short        INT16;      /* 16 bit Signed   Integer  */
typedef     unsigned short      UINT16;     /* 16 bit Unsigned Integer  */
typedef     int                 INT32;      /* 32 bit Signed   Integer  */
typedef     unsigned int        UINT32;     /* 32 bit Unsigned Integer  */
typedef     float               FLOAT32;    /* 32 bit Float             */
typedef     double              FLOAT64;    /* 64 bit Float             */
#ifndef OBJC_BOOL_DEFINED
typedef     int                 BOOL;
#endif
typedef     unsigned char       BYTE;
typedef     unsigned short      WORD;
typedef     unsigned long       DWORD;
typedef     unsigned char       RAWIMAGE;   /* Raw Image Data */

#ifndef     FALSE
    #define     FALSE               0
#endif

#ifndef     TRUE
    #define     TRUE                1
#endif

#ifndef     NULL
    #define     NULL                0
#endif

/* flag ON/OFF */
#ifndef     FLAG_OFF
    #define     FLAG_OFF            0       /* flag OFF */
#endif
#ifndef     FLAG_ON
    #define     FLAG_ON             1       /* flag ON  */
#endif


#if (!defined( WIN32 ) && !defined( WIN64 ))
    #define     VOID                void
    typedef     size_t              SIZE_T;

    typedef struct tagPOINT {
        INT32   x;
        INT32   y;
    } POINT;

    typedef struct tagRECT {
        INT32   left;
        INT32   top;
        INT32   right;
        INT32   bottom;
    } RECT;
#endif  /* WIN32 && WIN64 */

typedef struct tagFACEINFO {    /* Face Detection Information */
    INT32           nID;            /* ID Number */
    POINT           ptLeftTop;      /* Left-Top     Face Coordinates    */
    POINT           ptRightTop;     /* Right-Top    Face Coordinates    */
    POINT           ptLeftBottom;   /* Left-Bottom  Face Coordinates    */
    POINT           ptRightBottom;  /* Right-Bottom Face Coordinates    */
    INT32           nPose;          /* Face Pose                        */
    INT32           nConfidence;    /* Confidence Degree                */
} FACEINFO;

typedef enum tagYUV420_FORMAT {
    YUV420_FORMAT_CBCR = 0,
    YUV420_FORMAT_CRCB = 1
} YUV420_FORMAT;

#endif  /* OKAODEF_H__ */
