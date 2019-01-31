/*-------------------------------------------------------------------*/
/*  Copyright(C) 2003-2011 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* 
    Smile Degree Estimation Library API
*/

#ifndef OKAOSMAPI_H__
#define OKAOSMAPI_H__

#define OKAO_API
#include    "OkaoDef.h"

#ifndef OKAO_DEF_HSMILE
#define OKAO_DEF_HSMILE
typedef void *  HSMILE;         /* Smile Degree Estimation handle */
#endif /* OKAO_DEF_HSMILE */

#ifndef OKAO_DEF_HSMRESULT
#define OKAO_DEF_HSMRESULT
typedef void *  HSMRESULT;      /* Smile Degree Estimation result handle */
#endif /* OKAO_DEF_HSMRESULT */

#ifndef OKAO_DEF_HPTRESULT
#define OKAO_DEF_HPTRESULT
typedef void *  HPTRESULT;      /* Facial Parts Detection result handle */
#endif /* OKAO_DEF_HPTRESULT */


/*----- Index of Facial parts -----*/
enum SM_PTPOINT {
    SM_PTPOINT_LEFT_EYE_IN = 0,     /* Left eye inside        */
    SM_PTPOINT_LEFT_EYE_OUT,        /* Left eye outside       */
    SM_PTPOINT_RIGHT_EYE_IN,        /* Right eye inside       */
    SM_PTPOINT_RIGHT_EYE_OUT,       /* Right eye outside      */
    SM_PTPOINT_MOUTH_LEFT,          /* Mouth left             */
    SM_PTPOINT_MOUTH_RIGHT,         /* Mouth right            */
    SM_PTPOINT_MAX                  /* Number of Facial parts */
};


#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Get Version                                            */
/**********************************************************/
/* Get Smile Degree Estimation Library API Version */
OKAO_API INT32      OKAO_SM_GetVersion(UINT8 *pbyMajor, UINT8 *pbyMinor);

/**********************************************************/
/* Create/Delete Handle                                   */
/**********************************************************/
/* Create Smile Estimation handle */
OKAO_API HSMILE     OKAO_SM_CreateHandle(void);

/* Delete Smile Degree Estimation handle */
OKAO_API INT32      OKAO_SM_DeleteHandle(HSMILE hSM);

/* Create Smile Degree Estimation result handle */
OKAO_API HSMRESULT  OKAO_SM_CreateResultHandle(void);

/* Delete Smile Degree Estimation result handle */
OKAO_API INT32      OKAO_SM_DeleteResultHandle(HSMRESULT hSmResult);

/**********************************************************/
/* Facial Feature Point Setting                           */
/**********************************************************/
/* Set the feature points for Smile Degree Estimation */
OKAO_API INT32      OKAO_SM_SetPoint(HSMILE hSM, INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[], INT32 nUpDown, INT32 nLeftRight);

/* Set the feature points for Smile Degree Estimation from PT result handle */
OKAO_API INT32      OKAO_SM_SetPointFromHandle(HSMILE hSM, HPTRESULT hPtResult);

/**********************************************************/
/* Smile Degree Estimation                                */
/**********************************************************/
/* Estimate the smile degree */
OKAO_API INT32      OKAO_SM_Estimate(HSMILE hSM, RAWIMAGE *pImage, INT32 nWidth, INT32 nHeight, HSMRESULT hSmResult);

/**********************************************************/
/* Smile Degree Estimation Result                         */
/**********************************************************/
/* Get the estimated smile degree and its confidence level */
OKAO_API INT32      OKAO_SM_GetResult(HSMRESULT hSmResult, INT32 *pnSmile, INT32 *pnConfidence);

#ifdef  __cplusplus
}
#endif


#endif  /* OKAOSMAPI_H__ */
