/*-------------------------------------------------------------------*/
/*  Copyright(C) 2011-2012 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*
   Eye Detection Library Ver.1
*/
#ifndef OKAOEDAPI_H__
#define OKAOEDAPI_H__

#define OKAO_API
#include    "CommonDef.h"
#include    "OkaoDef.h"
#include    "DetectionInfo.h"
#include    "DetectorComDef.h"


#ifndef OKAO_DEF_HEYEDETECTION
    #define OKAO_DEF_HEYEDETECTION
    typedef void * HEYEDETECTION;       /* Eye Detection handle  */
    typedef void * HEDRESULT;           /* Eye Detection Result handle */
#endif /* OKAO_DEF_HEYEDETECTION */

#define DETECTOR_TYPE_SOFT_ED_V1   (DET_SOFT|DET_ED|DET_V1)  /* Eye Detection V1 */

#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Version infomation                                     */
/**********************************************************/
/* Gets Version */
OKAO_API INT32     OKAO_ED_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);


/**********************************************************/
/* Handle Creation/Deletion                               */
/**********************************************************/
/* Creates/Deletes Detection Handle */
OKAO_API HEYEDETECTION  OKAO_ED_CreateHandle( void );
OKAO_API INT32          OKAO_ED_DeleteHandle(HEYEDETECTION hED);

/* Creates/Deletes Detection Result Handle */
OKAO_API HEDRESULT OKAO_ED_CreateResultHandle(void);
OKAO_API INT32     OKAO_ED_DeleteResultHandle(HEDRESULT hEdResult);

/**********************************************************/
/* Excecution of Detection                                */
/**********************************************************/
/* Executes detection */
OKAO_API INT32     OKAO_ED_Detect(HEYEDETECTION hED, RAWIMAGE *pImage, INT32 nWidth, INT32 nHeight,
                                   volatile INT32 *pnBreakFlag, HEDRESULT hEdResult);


/**********************************************************/
/* Gets Detection Result                                  */
/**********************************************************/
/* Gets the number of detected eye */
OKAO_API INT32     OKAO_ED_GetResultCount(HEDRESULT hEdResult, INT32 *pnCount);

/* Gets the detection result for each eye */
OKAO_API INT32     OKAO_ED_GetResultInfo(HEDRESULT hEdResult,
                                          INT32 nIndex, DETECTION_INFO *psDetectionInfo);


/*************************************************************/
/* Setting functions                                         */
/*************************************************************/
/* Sets/Gets the min and max eye size */
OKAO_API INT32     OKAO_ED_SetSizeRange(HEYEDETECTION hED, INT32 nMinSize, INT32 nMaxSize);
OKAO_API INT32     OKAO_ED_GetSizeRange(HEYEDETECTION hED, INT32 *pnMinSize, INT32 *pnMaxSize);

/* Sets/Gets the direction to be detected */
OKAO_API INT32     OKAO_ED_SetAngle(HEYEDETECTION hED, UINT32 nAngle);
OKAO_API INT32     OKAO_ED_GetAngle(HEYEDETECTION hED, UINT32 *pnAngle);

/* Sets/Gets the edge mask area */
OKAO_API INT32     OKAO_ED_SetEdgeMask(HEYEDETECTION hED, RECT rcEdgeMask);
OKAO_API INT32     OKAO_ED_GetEdgeMask(HEYEDETECTION hED, RECT *prcEdgeMask);

/* Sets/Gets the Detection Threshold */
OKAO_API INT32     OKAO_ED_SetThreshold(HEYEDETECTION hED, INT32 nThreshold);
OKAO_API INT32     OKAO_ED_GetThreshold(HEYEDETECTION hED, INT32 *pnThreshold);


#ifdef  __cplusplus
}
#endif

#endif /* OKAOEDAPI_H__ */
