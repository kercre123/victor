/*--------------------------------------------------------------------*/
/*  Copyright(C) 2011 by OMRON Corporation                            */
/*  All Rights Reserved.                                              */
/*                                                                    */
/*     This software is the Confidential and Proprietary product      */
/*     of OMRON Corporation. Any unauthorized use, reproduction       */
/*     or transfer of this software is strictly prohibited.           */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*
   Pet Detection Library Ver.2
*/
#ifndef OMCVPDAPI_H__
#define OMCVPDAPI_H__

#define OMCV_API
#include    "CommonDef.h"
#include    "OkaoDef.h"
#include    "DetectionInfo.h"
#include    "DetectorComDef.h"


#ifndef OMCV_DEF_HPET
    #define OMCV_DEF_HPET
    typedef void * HPET;           /* Pet Detection handle  */
    typedef void * HPDRESULT;      /* Pet Detection Result handle */
#endif /* OMCV_DEF_HPET */

#define DETECTOR_TYPE_SOFT_PD_V2   (DET_SOFT|DET_PD|DET_V2)  /* Pet Detection V2 */

/* Search Density */
#define PD_DENSITY_HIGH_190         (190)
#define PD_DENSITY_HIGH_165         (165)
#define PD_DENSITY_NORMAL           (100)
#define PD_DENSITY_LOW_75            (75)
#define PD_DENSITY_LOW_50            (50)

#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Version infomation                                     */
/**********************************************************/
/* Gets Version */
OMCV_API INT32     OMCV_PD_GetVersion( UINT8 *pucMajor, UINT8 *pucMinor);


/**********************************************************/
/* Handle Creation/Deletion                               */
/**********************************************************/
/* Creates/Deletes Detection Handle */
OMCV_API HPET      OMCV_PD_CreateHandle( INT32 nDetectionMode, INT32 nMaxDetectionCount);
OMCV_API INT32     OMCV_PD_DeleteHandle( HPET hPD);

/* Creates/Deletes Detection Result Handle */
OMCV_API HPDRESULT OMCV_PD_CreateResultHandle( void );
OMCV_API INT32     OMCV_PD_DeleteResultHandle( HPDRESULT hPdResult);


/**********************************************************/
/* Excecution of Detection/Tracking                       */
/******************************************************** */
/* Executes Pet detection */
OMCV_API INT32     OMCV_PD_Detect( HPET hPD, RAWIMAGE *pImage, INT32 nWidth, INT32 nHeight,
                                      HPDRESULT hPdResult);


/**********************************************************/
/* Gets Detection Result                                  */
/**********************************************************/
/* Gets the number of detected pet */
OMCV_API INT32     OMCV_PD_GetResultCount( HPDRESULT hPdResult, INT32 nObjectType, INT32 *pnCount);

/* Gets the detection result for each pet */
OMCV_API INT32     OMCV_PD_GetResultInfo( HPDRESULT hPdResult, INT32 nObjectType,
                                          INT32 nIndex, DETECTION_INFO *psDetectionInfo);


/*************************************************************/
/* Setting functions for Still and Movie mode (COMMON)       */
/*************************************************************/

/* Sets/Gets the min and max pet size */
OMCV_API INT32     OMCV_PD_SetSizeRange( HPET hPD, INT32 nMinSize, INT32 nMaxSize);
OMCV_API INT32     OMCV_PD_GetSizeRange( HPET hPD, INT32 *pnMinSize, INT32 *pnMaxSize);

/* Sets/Gets the direction to be detected */
OMCV_API INT32     OMCV_PD_SetAngle( HPET hPD, INT32 nPose, UINT32 nAngle);
OMCV_API INT32     OMCV_PD_GetAngle( HPET hPD, INT32 nPose, UINT32 *pnAngle);

/* Sets/Gets the edge mask area */
OMCV_API INT32     OMCV_PD_SetEdgeMask( HPET hPD, RECT rcEdgeMask);
OMCV_API INT32     OMCV_PD_GetEdgeMask( HPET hPD, RECT *prcEdgeMask);

/* Sets/Gets search density */
OMCV_API INT32     OMCV_PD_SetSearchDensity(HPET hPD, INT32 nSearchDensity);
OMCV_API INT32     OMCV_PD_GetSearchDensity(HPET hPD, INT32 *pnSearchDensity);

/* Sets/Gets the Detection Threshold */
OMCV_API INT32     OMCV_PD_SetThreshold( HPET hPD, INT32 nThreshold);
OMCV_API INT32     OMCV_PD_GetThreshold( HPET hPD, INT32 *pnThreshold);

/* Sets/Gets the timeout for OMCV_PD_Detect() */
OMCV_API INT32     OMCV_PD_SetTimeout( HPET hPD, INT32 nTimeout);
OMCV_API INT32     OMCV_PD_GetTimeout( HPET hPD, INT32 *pnTimeout);


/*************************************************************/
/* Setting functions for Movie mode only (MOVIE MODE only)   */
/*************************************************************/
/* Clear pre-tracking results to be used for next tracking */
OMCV_API INT32     OMCV_PD_MV_ResetTracking( HPET hPD);

/* Sets/Gets search cycle */
OMCV_API INT32     OMCV_PD_MV_SetSearchCycle(HPET hPD,
                                     INT32 nInitialPetSearchCycle, INT32 nNewPetSearchCycle);
OMCV_API INT32     OMCV_PD_MV_GetSearchCycle(HPET hPD,
                                     INT32 *pnInitialPetSearchCycle, INT32 *pnNewPetSearchCycle);

/* Sets/Gets Max Retry Count and Max Hold Count */
OMCV_API INT32     OMCV_PD_MV_SetLostParam( HPET hPD, INT32 nMaxRetryCount, INT32 nMaxHoldCount);
OMCV_API INT32     OMCV_PD_MV_GetLostParam( HPET hPD, INT32 *pnMaxRetryCount, INT32 *pnMaxHoldCount);

/* Sets/Gets the steadiness parameters for size and positon of the detection result */
OMCV_API INT32     OMCV_PD_MV_SetSteadinessParam(HPET hPD, INT32 nPosSteadinessParam, INT32 nSizeSteadinessParam);
OMCV_API INT32     OMCV_PD_MV_GetSteadinessParam(HPET hPD, INT32 *pnPosSteadinessParam, INT32 *pnSizeSteadinessParam);


#ifdef  __cplusplus
}
#endif

#endif /* OMCVPDAPI_H__ */
