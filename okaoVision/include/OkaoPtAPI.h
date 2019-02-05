/*-------------------------------------------------------------------*/
/*  Copyright(C) 2003-2015 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* 
    Facial Parts Detection Library API
*/

#ifndef OKAOPTAPI_H__
#define OKAOPTAPI_H__

#define OKAO_API
#include "OkaoCoDef.h"

#ifndef OKAO_DEF_HCOMMON
#define OKAO_DEF_HCOMMON
    typedef VOID*   HCOMMON;
#endif /* OKAO_DEF_HCOMMON */

#ifndef OKAO_DEF_HPOINTER
#define OKAO_DEF_HPOINTER
    typedef VOID *      HPOINTER; /* Facial Parts Detection Handle */
#endif /* OKAO_DEF_HPOINTER */

#ifndef OKAO_DEF_HPTRESULT
#define OKAO_DEF_HPTRESULT
    typedef VOID *      HPTRESULT; /* Facial Parts Detection Result Handle */
#endif /* OKAO_DEF_HPTRESULT */

#ifndef OKAO_DEF_HDTRESULT
#define OKAO_DEF_HDTRESULT
    typedef VOID *      HDTRESULT; /* Face Detection Result Handle */
#endif /* OKAO_DEF_HDTRESULT */

/* Feature point extraction mode */
#define PT_MODE_DEFAULT             0                   /* Normal Mode      */
#define PT_MODE_FAST                1                   /* Fast Mode        */
#define PT_MODE_SUPER_FAST          2                   /* Super Fast Mode  */

/* Confidence output mode */
#define PT_CONF_USE                 0                   /* Enable confidence calculation */
#define PT_CONF_NOUSE               1                   /* Skip Confidence calculation   */

#define PT_MODE_CONF_DEFAULT        PT_CONF_USE         /* Enable confidence calculation */
#define PT_MODE_CONF_NOUSE          PT_CONF_NOUSE       /* Skip Confidence calculation   */

/* Face detection (FD) version */
#define DTVERSION  INT32
#define DTVERSION_SOFT_V3       300   /* FD V3.x(000300)               */
#define DTVERSION_SOFT_V4       400   /* FD V4.x(000400)               */
#define DTVERSION_SOFT_V5       500   /* FD V5.x(000500)               */
#define DTVERSION_SOFT_V6       600   /* FD V6.x(000600)               */
#define DTVERSION_IP_V1       10100   /* FD IPV1.x                     */
#define DTVERSION_IP_V2       10200   /* FD IPV2.x                     */
#define DTVERSION_IP_V3       10300   /* FD IPV3.x                     */
#define DTVERSION_IP_V4       10400   /* FD IPV4.x                     */
#define DTVERSION_MD_V1       20100   /* MD IPV1.x                     */
#define DTVERSION_UNKNOWN         0   /* 3rd party FD engine (000000)  */

/* Detected face pose */
#define PT_POSE_LF_PROFILE   -90    /* Left side face               */
#define PT_POSE_LH_PROFILE   -45    /* Partially left sided face    */
#define PT_POSE_FRONT          0    /* Frontal face                 */
#define PT_POSE_RH_PROFILE    45    /* Partially Right sided face   */
#define PT_POSE_RF_PROFILE    90    /* Right sided face             */
#define PT_POSE_HEAD        -180    /* Head                         */
#define PT_POSE_UNKNOWN        0    /* Pose Unknown                 */

/* Facial Parts Detection Point Index */
enum PT_PARTS_POINT {
    PT_POINT_LEFT_EYE = 0,  /* Center of left eye        */
    PT_POINT_RIGHT_EYE,     /* Center of right eye       */
    PT_POINT_MOUTH,         /* Mouth Center              */
    PT_POINT_LEFT_EYE_IN,   /* Inner corner of left eye  */
    PT_POINT_LEFT_EYE_OUT,  /* Outer corner of left eye  */
    PT_POINT_RIGHT_EYE_IN,  /* Inner corner of right eye */
    PT_POINT_RIGHT_EYE_OUT, /* Outer corner of right eye */
    PT_POINT_MOUTH_LEFT,    /* Left corner of mouth      */
    PT_POINT_MOUTH_RIGHT,   /* Right corner of mouth     */
    PT_POINT_NOSE_LEFT,     /* Left Nostril              */
    PT_POINT_NOSE_RIGHT,    /* Right Nostril             */
    PT_POINT_MOUTH_UP,      /* Mouth top                 */
    PT_POINT_KIND_MAX       /* Number of Parts Points    */
};


#define FEATURE_NO_POINT    -1  /* Parts Points Could Not Be Detected */

#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Get Version                                            */
/**********************************************************/
/* Get Facial Parts Detection Library API Version */
OKAO_API INT32      OKAO_PT_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);

/**********************************************************/
/* Create/Delete Handle                                   */
/**********************************************************/
/* Create Facial Parts Detection Handle */
OKAO_API HPOINTER   OKAO_PT_CreateHandle(HCOMMON hCO);
/* Delete Facial Parts Detection Handle */
OKAO_API INT32      OKAO_PT_DeleteHandle(HPOINTER hPT);
/* Create Facial Parts Detection Result Handle */
OKAO_API HPTRESULT  OKAO_PT_CreateResultHandle(HCOMMON hCO);
/* Delete Facial Parts Detection Result Handle */
OKAO_API INT32      OKAO_PT_DeleteResultHandle(HPTRESULT hPtResult);

/**********************************************************/
/* Set Face Position                                      */
/**********************************************************/
/* Set face position from Face detection result */
OKAO_API INT32      OKAO_PT_SetPositionFromHandle(HPOINTER hPT, HDTRESULT hDtResult,INT32 nIndex);
/* Set face position */
OKAO_API INT32      OKAO_PT_SetPosition(HPOINTER hPT, POINT *pptLeftTop, POINT *pptRightTop, POINT *pptLeftBottom,
                                                                       POINT *pptRightBottom, INT32 nPose, DTVERSION DtVersion);
/* Set face position from Motion face detection IP result */
OKAO_API INT32      OKAO_PT_SetPositionIP(HPOINTER hPT, INT32 nCenterX, INT32 nCenterY, INT32 nSize, INT32 nAngle,
                                                                       INT32 nScale, INT32 nPose, DTVERSION DtVersion);

/**********************************************************/
/* Set/Get Facial Parts Detection Mode                    */
/**********************************************************/
/* Set Facial Parts Detection Mode */
OKAO_API INT32      OKAO_PT_SetMode(HPOINTER hPT, INT32 nMode);
/* Get Facial Parts Detection Mode */
OKAO_API INT32      OKAO_PT_GetMode(HPOINTER hPT, INT32 *pnMode);

/**********************************************************/
/* Set/Get Confidence calculation Mode                    */
/**********************************************************/
/* Set Confidence calculation Mode */
OKAO_API INT32      OKAO_PT_SetConfMode(HPOINTER hPT, INT32 nConfMode);
/* Get Confidence calculation Mode */
OKAO_API INT32      OKAO_PT_GetConfMode(HPOINTER hPT, INT32 *pnConfMode);

/**********************************************************/
/* Facial Parts Detection                                 */
/**********************************************************/
/* Execute Facial Parts Detection(GRAY) */
OKAO_API INT32      OKAO_PT_DetectPoint_GRAY(HPOINTER hPT, RAWIMAGE *pImageGRAY, INT32 nWidth, INT32 nHeight,
                                                                        GRAY_ORDER ImageOrder, HPTRESULT hPtResult);
/* Execute Facial Parts Detection(YUV422) */
OKAO_API INT32      OKAO_PT_DetectPoint_YUV422(HPOINTER hPT, RAWIMAGE *pImageYUV, INT32 nWidth, INT32 nHeight,
                                                                        YUV422_ORDER ImageOrder, HPTRESULT hPtResult);
/* Execute Facial Parts Detection(YUV420SP) */
OKAO_API INT32      OKAO_PT_DetectPoint_YUV420SP(HPOINTER hPT, RAWIMAGE *pImageY, RAWIMAGE *pImageCx, INT32 nWidth, INT32 nHeight, 
                                                                        YUV420SP_ORDER ImageOrder, HPTRESULT hPtResult);
/* Execute Facial Parts Detection(YUV420FP) */
OKAO_API INT32      OKAO_PT_DetectPoint_YUV420FP(HPOINTER hPT, RAWIMAGE *pImageY, RAWIMAGE *pImageCb, RAWIMAGE *pImageCr, INT32 nWidth, INT32 nHeight,
                                                                        YUV420FP_ORDER ImageOrder, HPTRESULT hPtResult);

/**********************************************************/
/* Get Facial Parts Detection Result                      */
/**********************************************************/
/* Get Facial Parts Position Result */
OKAO_API INT32      OKAO_PT_GetResult(HPTRESULT hPtResult, INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[]);
/* Get the face direction angles(degree) detected by OKAO_Pointer() */
OKAO_API INT32      OKAO_PT_GetFaceDirection(HPTRESULT hPtResult, INT32 *pnUpDown, INT32 *pnLeftRight, INT32 *pnRoll);

#ifdef  __cplusplus
}
#endif

#endif    /* OKAOPTAPI_H__ */
