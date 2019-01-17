/*-------------------------------------------------------------------*/
/*  Copyright(C) 2010-2015 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*
    Face Recognition Library API
*/

#ifndef OKAOFRAPI_H__
#define OKAOFRAPI_H__

#define OKAO_API

#include    "OkaoCoDef.h"

#ifndef OKAO_DEF_HALBUM
#define OKAO_DEF_HALBUM
    typedef void*           HALBUM;         /* Album Data Handle */
#endif /* OKAO_DEF_HALBUM */

#ifndef OKAO_DEF_HFEATURE
#define OKAO_DEF_HFEATURE
    typedef void*           HFEATURE;       /* Face Feature Data Handle */
#endif /* OKAO_DEF_HFACEDATA */

#ifndef OKAO_DEF_HPTRESULT
#define OKAO_DEF_HPTRESULT
    typedef void*           HPTRESULT;
#endif /* OKAO_DEF_HPTRESULT */

#ifndef OKAO_DEF_HCOMMON
#define OKAO_DEF_HCOMMON
    typedef VOID*   HCOMMON;
#endif /* OKAO_DEF_HCOMMON */

#define SERIALIZED_FEATUR_MEM_SIZE 176   /* Size of Serialized Feature Memory */

typedef INT32 FR_ERROR;

#define     FR_NORMAL                         0      /* Normal End              */
#define     FR_ERR_INVALIDPARAM              -3      /* Invalid Parameter Error */
#define     FR_ERR_ALLOCMEMORY               -4      /* Memory Allocation Error */
#define     FR_ERR_NOHANDLE                  -7      /* Handle Error            */
#define     FR_ERR_UNKOWN_FORMAT           -100      /* Unknown Format          */
#define     FR_ERR_INVALID_DATA_VERSION    -101      /* Invalid Data Version    */
#define     FR_ERR_INVALID_DATA            -102      /* Invalid Data            */

/* Facial Parts Detection Point Index */
enum FR_PT_PARTS_POINT {
    FR_PTPOINT_LEFT_EYE = 0,              /* Left Eye Center    */
    FR_PTPOINT_RIGHT_EYE,                 /* Right Eye Center   */
    FR_PTPOINT_MOUTH,                     /* Mouth Center       */
    FR_PTPOINT_LEFT_EYE_IN,               /* Left Eye In        */
    FR_PTPOINT_LEFT_EYE_OUT,              /* Left Eye Out       */
    FR_PTPOINT_RIGHT_EYE_IN,              /* Right Eye In       */
    FR_PTPOINT_RIGHT_EYE_OUT,             /* Right Eye Out      */
    FR_PTPOINT_MOUTH_LEFT,                /* Mouth Left         */
    FR_PTPOINT_MOUTH_RIGHT,               /* Mouth Right        */
    FR_PTPOINT_NOSE_LEFT,                 /* Nose Left          */
    FR_PTPOINT_NOSE_RIGHT,                /* Nose Right         */
    FR_PTPOINT_MOUTH_UP,                  /* Mouth Up           */
    FR_PTPOINT_KIND_MAX                   /* The number of Feature Points */
};


#ifdef  __cplusplus
extern "C" {
#endif

/*********************************************
* Version Information
*********************************************/
/*       Gets Version       */
OKAO_API INT32 OKAO_FR_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);


/*********************************************
* Face Feature Data Handle Creation/Deletion
*********************************************/
/*      Creates Face Feature Data Handle    */
OKAO_API HFEATURE OKAO_FR_CreateFeatureHandle(HCOMMON hCO);
/*      Deletes Face Feature Data Handle    */
OKAO_API INT32 OKAO_FR_DeleteFeatureHandle(HFEATURE hFeature);


/*********************************************
* Face Feature Extraction
*********************************************/
/*      Extracts Face Feature from Facial Part Positions and the GRAY Image                     */
OKAO_API INT32 OKAO_FR_ExtractPoints_GRAY(HFEATURE hFeature, RAWIMAGE *pImageGRAY, INT32 nWidth, INT32 nHeight,
                                        GRAY_ORDER ImageOrder, INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[]);
/*      Extracts Face Feature from Facial Part Positions and the YUV422 Image                   */
OKAO_API INT32 OKAO_FR_ExtractPoints_YUV422(HFEATURE hFeature, RAWIMAGE *pImageYUV, INT32 nWidth, INT32 nHeight,
                                        YUV422_ORDER ImageOrder, INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[]);
/*      Extracts Face Feature from Facial Part Positions and the YUV420SP Image                 */
OKAO_API INT32 OKAO_FR_ExtractPoints_YUV420SP(HFEATURE hFeature, RAWIMAGE *pImageY, RAWIMAGE *pImageCx, INT32 nWidth, INT32 nHeight,
                                        YUV420SP_ORDER ImageOrder, INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[]);
/*      Extracts Face Feature from Facial Part Positions and the YUV420FP Image                 */
OKAO_API INT32 OKAO_FR_ExtractPoints_YUV420FP(HFEATURE hFeature, RAWIMAGE *pImageY, RAWIMAGE *pImageCb, RAWIMAGE *pImageCr,
                                        INT32 nWidth, INT32 nHeight, YUV420FP_ORDER ImageOrder,
                                        INT32 nPointNum, POINT aptPoint[], INT32 anConfidence[]);
/*      Extracts Face Feature from Facial Parts Detection Result Handle ans the GRAY Image      */
OKAO_API INT32 OKAO_FR_ExtractHandle_GRAY(HFEATURE hFeature, RAWIMAGE *pImageGRAY, INT32 nWidth, INT32 nHeight,
                                        GRAY_ORDER ImageOrder, HPTRESULT hPtResult);
/*      Extracts Face Feature from Facial Parts Detection Result Handle ans the YUV422 Image    */
OKAO_API INT32 OKAO_FR_ExtractHandle_YUV422(HFEATURE hFeature, RAWIMAGE *pImageYUV, INT32 nWidth, INT32 nHeight,
                                        YUV422_ORDER ImageOrder, HPTRESULT hPtResult);
/*      Extracts Face Feature from Facial Parts Detection Result Handle ans the YUV420SP Image  */
OKAO_API INT32 OKAO_FR_ExtractHandle_YUV420SP(HFEATURE hFeature, RAWIMAGE *pImageY, RAWIMAGE *pImageCx, INT32 nWidth, INT32 nHeight,
                                        YUV420SP_ORDER ImageOrder, HPTRESULT hPtResult);
/*      Extracts Face Feature from Facial Parts Detection Result Handle ans the YUV420FP Image  */
OKAO_API INT32 OKAO_FR_ExtractHandle_YUV420FP(HFEATURE hFeature, RAWIMAGE *pImageY, RAWIMAGE *pImageCb, RAWIMAGE *pImageCr,
                                        INT32 nWidth, INT32 nHeight, YUV420FP_ORDER ImageOrder, HPTRESULT hPtResult);


/*********************************************
* Face Feature Data Reading/Writing
*********************************************/
/*      Writes Face Feature Data into Memory     */
OKAO_API INT32 OKAO_FR_WriteFeatureToMemory(HFEATURE hFeature, UINT8 *pbyBuffer, UINT32 unBufSize);
/*      Reads Face Feature Data from Memory      */
OKAO_API INT32 OKAO_FR_ReadFeatureFromMemory(HFEATURE hFeature, UINT8 *pbyBuffer, UINT32 unBufSize, FR_ERROR *pError);


/*********************************************
* Album Data Handle Creation/Deletion
*********************************************/
/*      Creates Album Data Handle   */
OKAO_API HALBUM OKAO_FR_CreateAlbumHandle(HCOMMON hCO, INT32 nMaxUserNum, INT32 nMaxDataNumPerUser);
/*      Deletes Album Data Handle   */
OKAO_API INT32 OKAO_FR_DeleteAlbumHandle(HALBUM hAlbum);


/*********************************************
* Acquisition of Album Data configuration
*********************************************/
/*      Gets the maximal numbers of Users and Feature Data per User */
OKAO_API INT32 OKAO_FR_GetAlbumMaxNum(HALBUM hAlbum, INT32 *pnMaxUserNum, INT32 *pnMaxDataNumPerUser);


/*********************************************
* Data Registration In Album
*********************************************/
/*      Registers Face Feature Data into Album Data */
OKAO_API INT32 OKAO_FR_RegisterData(HALBUM hAlbum, HFEATURE hFeature, INT32 nUserID, INT32 nDataID);


/*********************************************
* Acquisition of Album Data Registration Status
*********************************************/
/*      Gets the total number of Data registered in Album Data          */
OKAO_API INT32 OKAO_FR_GetRegisteredAllDataNum(HALBUM hAlbum, INT32 *pnAllDataNum);
/*      Gets the number of Users with registered Data                   */
OKAO_API INT32 OKAO_FR_GetRegisteredUserNum(HALBUM hAlbum, INT32 *pnUserNum);
/*      Gets the number of registered Feature Data of a specified User  */
OKAO_API INT32 OKAO_FR_GetRegisteredUsrDataNum(HALBUM hAlbum, INT32 nUserID, INT32 *pnUserDataNum);
/*      Gets Data Registration Status of a specified User               */
OKAO_API INT32 OKAO_FR_IsRegistered(HALBUM hAlbum, INT32 nUserID, INT32 nDataID, BOOL *pIsRegistered);


/*********************************************
* Clearance of Album Data
*********************************************/
/*      Clears all the Data from Album Data                         */
OKAO_API INT32 OKAO_FR_ClearAlbum(HALBUM hAlbum);
/*      Clears all the Data of a specified User from Album Data     */
OKAO_API INT32 OKAO_FR_ClearUser(HALBUM hAlbum, INT32 nUserID);
/*      Clears a specified Data of a specified User from Album Data */
OKAO_API INT32 OKAO_FR_ClearData(HALBUM hAlbum, INT32 nUserID, INT32 nDataID);


/*********************************************
* Album Serialization/Restoration
*********************************************/
/*      Gets the size of Serialized Album Data  */
OKAO_API INT32 OKAO_FR_GetSerializedAlbumSize(HALBUM hAlbum, UINT32 *punSerializedAlbumSize);
/*      Serializes Album Data                   */
OKAO_API INT32 OKAO_FR_SerializeAlbum(HALBUM hAlbum, UINT8 *pbyBuffer, UINT32 unBufSize);
/*      Restores Album Data                     */
OKAO_API HALBUM OKAO_FR_RestoreAlbum(HCOMMON hCO, UINT8 *pbyBuffer, UINT32 unBufSize, FR_ERROR *pError);


/*********************************************
* Acquisition of Feature Data from Album Data
*********************************************/ 
/*      Gets Feature Data from Album Data      */
OKAO_API INT32 OKAO_FR_GetFeatureFromAlbum(HALBUM hAlbum, INT32 nUserID, INT32 nDataID, HFEATURE hFeature);


/*********************************************
* Verification/Identification
*********************************************/
/*      Verification      */
OKAO_API INT32 OKAO_FR_Verify(HFEATURE hFeature, HALBUM hAlbum, INT32 nUserID, INT32 *pnScore );
/*      Identification    */
OKAO_API INT32 OKAO_FR_Identify(HFEATURE hFeature, HALBUM hAlbum, INT32 nMaxResultNum,
                                        INT32 anUserID[], INT32 anScore[], INT32 *pnResultNum);

#ifdef  __cplusplus
}
#endif

#endif  /* OKAOFRAPI_H__ */

