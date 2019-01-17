/******************************************************************************/
/*  Copyright(C) 2010 by OMRON Corporation                                    */
/*  All Rights Reserved.                                                      */
/******************************************************************************/
/**
 *  Gaze Blink Estimation Library API
 */
#ifndef OKAOGBAPI_H__
#define OKAOGBAPI_H__

#define OKAO_API

#include    "OkaoDef.h"


#ifndef OKAO_DEF_HGAZEBLINK
#define OKAO_DEF_HGAZEBLINK
typedef VOID * HGAZEBLINK;    /* Gaze Blink Estimation handle       */
#endif /* OKAO_DEF_HGAZEBLINK */

#ifndef OKAO_DEF_HGBRESULT
#define OKAO_DEF_HGBRESULT
typedef VOID * HGBRESULT;     /* Gaze Blink Estimation Result handle */
#endif /* OKAO_DEF_HGBRESULT */

#ifndef OKAO_DEF_HPTRESULT
#define OKAO_DEF_HPTRESULT
typedef VOID * HPTRESULT;
#endif /* OKAO_DEF_HPTRESULT */

/*----- Index of Facial parts -----*/
enum GB_PTPOINT {
    GB_PTPOINT_LEFT_EYE_IN = 0,     /* Left eye inside        */
    GB_PTPOINT_LEFT_EYE_OUT,        /* Left eye outside       */
    GB_PTPOINT_RIGHT_EYE_IN,        /* Right eye inside       */
    GB_PTPOINT_RIGHT_EYE_OUT,       /* Right eye outside      */
    GB_PTPOINT_MAX                  /* Number of Facial parts */
};

#ifdef  __cplusplus
extern "C" {
#endif


/**********************************************************/
/* Get Version                                            */
/**********************************************************/
/* Get Version */
OKAO_API INT32        OKAO_GB_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);


/**********************************************************/
/* Create/Delete Handle                                   */
/**********************************************************/
/* Create/Delete Gaze Blink Estimation handle */
OKAO_API HGAZEBLINK   OKAO_GB_CreateHandle(void);
OKAO_API INT32        OKAO_GB_DeleteHandle(HGAZEBLINK hGB);

/* Create/Delete Gaze Blink Estimation result handle */
OKAO_API HGBRESULT    OKAO_GB_CreateResultHandle(void);
OKAO_API INT32        OKAO_GB_DeleteResultHandle(HGBRESULT hGbResult);

/**********************************************************/
/* Set Facial Parts Position                              */
/**********************************************************/
/* Set facial parts postion */
OKAO_API INT32        OKAO_GB_SetPoint(HGAZEBLINK hGB, INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[],
                                       INT32 nUpDown, INT32 nLeftRight);

/* Set facial parts position from PT result handle */
OKAO_API INT32        OKAO_GB_SetPointFromHandle(HGAZEBLINK hGB, HPTRESULT hPtResult);

/**********************************************************/
/* Gaze Blink Estimation                                  */
/******************************************************** */
/* Execute Gaze Blink Estimation */
OKAO_API INT32        OKAO_GB_Estimate(HGAZEBLINK hGB, RAWIMAGE *pImage, INT32 nWidth, INT32 nHeight, HGBRESULT hGbResult);

/**********************************************************/
/* Get Gaze Blink Estimation Result                       */
/**********************************************************/
/* Get the estimation result for blink */
OKAO_API INT32        OKAO_GB_GetEyeCloseRatio(HGBRESULT hGbResult, INT32 *pnCloseRatioLeftEye, INT32 *pnCloseRatioRightEye);
/* Get the estimation result for gaze */
OKAO_API INT32        OKAO_GB_GetGazeDirection(HGBRESULT hGbResult, INT32 *pnGazeLeftRight, INT32 *pnGazeUpDown);


#ifdef  __cplusplus
}
#endif

#endif /* OKAOGBAPI_H__ */
