/*-------------------------------------------------------------------*/
/*  Copyright(C) 2003-2013 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* 
    OKAO_SDK Library API
*/
#ifndef OKAOCOAPI_H__
#define OKAOCOAPI_H__

#define OKAO_API

#include "OkaoCoDef.h"

#ifndef OKAO_DEF_HCOMMON
#define OKAO_DEF_HCOMMON
    typedef VOID*   HCOMMON;
#endif /* OKAO_DEF_HCOMMON */

#ifdef  __cplusplus
extern "C" {
#endif

/************************************************************/
/* Get Version                                              */
/************************************************************/
/* Get Version */
OKAO_API INT32      OKAO_CO_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);

/************************************************************/
/* Common Function Handle Creation/Deletion                 */
/************************************************************/
/* Creation */
OKAO_API HCOMMON    OKAO_CO_CreateHandle(void);

OKAO_API HCOMMON    OKAO_CO_CreateHandleMalloc(void *(*malloc)(size_t size),
                                               void (*free)(void *));

OKAO_API HCOMMON    OKAO_CO_CreateHandleMemory(VOID *pBMemoryAddr, UINT32 unBMemorySize,
                                               VOID *pWMemoryAddr, UINT32 unWMemorySize);
/* Deletion */
OKAO_API INT32      OKAO_CO_DeleteHandle(HCOMMON hCO);

/************************************************************/
/* Square Points to Center-Form                             */
/************************************************************/
/* Conversion from Square Points to Center-Form */
OKAO_API INT32      OKAO_CO_ConvertSquareToCenter(POINT ptLeftTop,
                                                  POINT ptRightTop,
                                                  POINT ptLeftBottom,
                                                  POINT ptRightBottom,
                                                  POINT *pptCenter,
                                                  INT32 *pnSize,
                                                  INT32 *pnAngle);

/************************************************************/
/* Center-Form to Square Points                             */
/************************************************************/
/* Convertsion from Center-Form to Square Points */
OKAO_API INT32      OKAO_CO_ConvertCenterToSquare(POINT ptCenter,
                                                  INT32 nSize,
                                                  INT32 nAngle,
                                                  POINT *pptLeftTop,
                                                  POINT *pptRightTop,
                                                  POINT *pptLeftBottom,
                                                  POINT *pptRightBottom);

#ifdef  __cplusplus
}
#endif

#endif  /* OKAOCOAPI_H__ */
