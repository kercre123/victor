/****************************************************************************\
* Copyright (C) 2016 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

/**
* \addtogroup royaleCAPI
* @{
*/

#pragma once

#ifdef __cplusplus
#define ROYALE_CAPI_LINKAGE_TOP extern "C" {
#define ROYALE_CAPI_LINKAGE_BOTTOM }
#else
#define ROYALE_CAPI_LINKAGE_TOP
#define ROYALE_CAPI_LINKAGE_BOTTOM
#endif

#ifdef _WIN32
#   ifdef ROYALE_CAPI_EXPORTS
#       define ROYALE_CAPI __declspec(dllexport)
#   else
#       define ROYALE_CAPI __declspec(dllimport)
#   endif   //ROYALE_CAPI_EXPORTS
#define ROYALE_CAPI_DEPRECATED ROYALE_CAPI __declspec(deprecated)
#else
#    define ROYALE_CAPI
#    define ROYALE_CAPI_DEPRECATED ROYALE_CAPI __attribute__((deprecated))
#endif  //_WIN32

/** @}*/
