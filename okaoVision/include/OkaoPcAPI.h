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
    Property Estimation Common Library API
*/

#ifndef OKAOPCAPI_H__
#define OKAOPCAPI_H__

#define OKAO_API
#include    "OkaoDef.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Get Version                                            */
/**********************************************************/
/* Get Property Estimation Common Library API Version */
OKAO_API INT32      OKAO_PC_GetVersion(UINT8 *pbyMajor, UINT8 *pbyMinor);

#ifdef  __cplusplus
}
#endif


#endif /* OKAOPCAPI_H__ */
