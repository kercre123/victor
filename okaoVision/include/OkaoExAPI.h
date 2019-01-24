/*-------------------------------------------------------------------*/
/*  Copyright(C) 2012-2014 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*
    Expression Estimation Library Ver.1.1
*/

#ifndef OKAOEXAPI_H__
#define OKAOEXAPI_H__

#define OKAO_API
#include "OkaoCoDef.h"

#ifndef OKAO_DEF_HCOMMON
#define OKAO_DEF_HCOMMON
    typedef VOID*   HCOMMON;
#endif /* OKAO_DEF_HCOMMON */

#ifndef OKAO_DEF_HEXPRESSION
#define OKAO_DEF_HEXPRESSION
    typedef VOID*   HEXPRESSION;      /* Expression Estimation handle */
#endif /* OKAO_DEF_HEXPRESSION */

#ifndef OKAO_DEF_HEXRESULT
#define OKAO_DEF_HEXRESULT
    typedef VOID*   HEXRESULT;        /* Expression Estimation Result handle */
#endif /* OKAO_DEF_HEXRESULT */

#ifndef OKAO_DEF_HPTRESULT
#define OKAO_DEF_HPTRESULT
    typedef VOID*   HPTRESULT;        /* Facial Parts Detection result handle */
#endif /* OKAO_DEF_HPTRESULT */

/* Estiomation Result */
enum EX_EXPRESSION {
    EX_EXPRESSION_NEUTRAL = 0, /* Neutral */
    EX_EXPRESSION_HAPPINESS,   /* Happiness */
    EX_EXPRESSION_SURPRISE,    /* Surpprise*/
    EX_EXPRESSION_ANGER,       /* Anger */
    EX_EXPRESSION_SADNESS,     /* Sadness */
    EX_EXPRESSION_KIND_MAX     /* Number of expression categories */
};

/* Facial Parts Detection Point Index */
enum EX_PT_PARTS_POINT {
    EX_PTPOINT_LEFT_EYE = 0,  /* Center of left eye        */
    EX_PTPOINT_RIGHT_EYE,     /* Center of right eye       */
    EX_PTPOINT_MOUTH,         /* Mouth Center              */
    EX_PTPOINT_LEFT_EYE_IN,   /* Inner corner of left eye  */
    EX_PTPOINT_LEFT_EYE_OUT,  /* Outer corner of left eye  */
    EX_PTPOINT_RIGHT_EYE_IN,  /* Inner corner of right eye */
    EX_PTPOINT_RIGHT_EYE_OUT, /* Outer corner of right eye */
    EX_PTPOINT_MOUTH_LEFT,    /* Left corner of mouth      */
    EX_PTPOINT_MOUTH_RIGHT,   /* Right corner of mouth     */
    EX_PTPOINT_NOSE_LEFT,     /* Left Nostril              */
    EX_PTPOINT_NOSE_RIGHT,    /* Right Nostril             */
    EX_PTPOINT_MOUTH_UP,      /* Mouth top                 */
    EX_PTPOINT_KIND_MAX       /* Number of Parts Points    */
};


#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Version infomation                                     */
/**********************************************************/
/* Gets Expression Estimation Library API Version */
OKAO_API INT32 OKAO_EX_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);


/**********************************************************/
/* Handle Creation/Deletion                               */
/**********************************************************/
/* Creates Expression Estimation Handle */
OKAO_API HEXPRESSION OKAO_EX_CreateHandle(HCOMMON hCO);
/* Deletes Expression Estimation Handle */
OKAO_API INT32 OKAO_EX_DeleteHandle(HEXPRESSION hEX);
/* Creates Expression Estimation Result Handle */
OKAO_API HEXRESULT OKAO_EX_CreateResultHandle(HCOMMON hCO);
/* Deletes Expression Estimation Result Handle */
OKAO_API INT32 OKAO_EX_DeleteResultHandle(HEXRESULT hExResult);


/**********************************************************/
/* Setting of Facial Parts Position                       */
/**********************************************************/
/* Sets the facial parts position */
OKAO_API INT32 OKAO_EX_SetPoint(HEXPRESSION hEX, INT32 nPointNum, POINT aptPoint[],
                                INT32 anConfidence[], INT32 nUpDown, INT32 nLeftRight);
/* Sets facial parts position from PT result handle */
OKAO_API INT32 OKAO_EX_SetPointFromHandle(HEXPRESSION hEX, HPTRESULT hPtResult);


/**********************************************************/
/* Excecution of Expression Estimation                    */
/**********************************************************/
/* Estimates the expression score */
OKAO_API INT32 OKAO_EX_Estimate_GRAY(HEXPRESSION hEX,
                                     RAWIMAGE    *pImageGRAY,
                                     INT32       nWidth,
                                     INT32       nHeight,
                                     GRAY_ORDER  ImageOrder,
                                     HEXRESULT   hExResult);

OKAO_API INT32 OKAO_EX_Estimate_YUV422(HEXPRESSION   hEX,
                                       RAWIMAGE      *pImageYUV,
                                       INT32         nWidth,
                                       INT32         nHeight,
                                       YUV422_ORDER  ImageOrder,
                                       HEXRESULT     hExResult);

OKAO_API INT32 OKAO_EX_Estimate_YUV420SP(HEXPRESSION     hEX,
                                         RAWIMAGE        *pImageY,
                                         RAWIMAGE        *pImageCx,
                                         INT32           nWidth,
                                         INT32           nHeight,
                                         YUV420SP_ORDER  ImageOrder,
                                         HEXRESULT       hExResult);

OKAO_API INT32 OKAO_EX_Estimate_YUV420FP(HEXPRESSION     hEX,
                                         RAWIMAGE        *pImageY,
                                         RAWIMAGE        *pImageCb,
                                         RAWIMAGE        *pImageCr,
                                         INT32           nWidth,
                                         INT32           nHeight,
                                         YUV420FP_ORDER  ImageOrder,
                                         HEXRESULT       hExResult);

/**********************************************************/
/* Get Expression Estimation Result                       */
/**********************************************************/
/* Gets score for each facial expression category */
OKAO_API INT32 OKAO_EX_GetResult(HEXRESULT hExResult, INT32 nExpressionNum, INT32 anScore[]);

#ifdef  __cplusplus
}
#endif


#endif  /* OKAOEXAPI_H__ */
