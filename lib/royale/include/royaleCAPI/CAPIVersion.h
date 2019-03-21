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

/*
 * C API version, selectable by setting ROYALE_C_API_VERSION to one of the following values:
 *
 * 220   - C API as of Royale v2.2.0
 * 300   - C API as of Royale v3.0.0 (includes support for mixed-mode)
 *         Mixed mode added StreamIds to many functions, including one callback (the exposure listener)
 * 320   - C API as of Royale v3.2.0
 *         added royale_get_version_with_build_and_scm_revision() (see royaleCAPI.h)
 *         added functions to shift and retrieve the lens center
 * 330   - C API as of Royale v3.3.0
 *         added royale_camera_device_get_number_of_streams
 *         added royale_camera_device_set_external_trigger
 * 31000 - C API as of Royale v3.10.0
 *         added royale_camera_device_write_data_to_flash
 *
 * Older C API versions were already deprecated in previous releases and have now been removed.
 *
 * Starting with Royale v3.0.0, C API compatibility is selected by defining ROYALE_C_API_VERSION
 * to one of the above values. To aid in migration, the latest API version is also made available
 * using alternative names (with a "_new_api" suffix added) if the preprocessor symbol
 * ROYALE_C_API_MIGRATION is defined.
 *
 * A possible migration strategy would be:
 * - define ROYALE_C_API_VERSION=220 and ROYALE_C_API_MIGRATION (to any value)
 * - compile the code. The compiler should warn about deprecated functions.
 * - fix all warnings by adapting the code to the new version, using the alternate function name
 *   with "_new_api" suffix
 * - after all warnings are fixed, unset ROYALE_C_API_MIGRATION and ROYALE_C_API_VERSION
 * - remove the "_new_api" suffix from the C API calls in the code
 *
 * In previous releases, the C API version offered could be selected using the macros
 * ROYALE_NEW_API_2_2_0 and ROYALE_FINAL_API_2_2_0. Defining any of these will now
 * lead to a compiler error (close to a piece of documentation telling you how to fix this).
 */

#ifndef ROYALE_C_API_VERSION
#define ROYALE_C_API_VERSION 31000
#endif

/* old, now unsupported way of selecting API versions: */
#if defined(ROYALE_NEW_API_2_2_0) && defined(ROYALE_FINAL_API_2_2_0) && \
    (ROYALE_C_API_VERSION == 220) && ! defined (ROYALE_C_API_MIGRATION)
/*
 * The application is using both the new and old ways of requesting the v2.2.0 API,
 * it's clear which API is needed and this doesn't trigger the #error that's in the
 * next section.
 *
 * This is supported so that an application written to the v2.2.0 API can have a
 * common code base for both the v3.0.0 branch and the long-term support branch.
 */
#elif defined(ROYALE_NEW_API_2_2_0) || defined(ROYALE_FINAL_API_2_2_0)
#error ROYALE_NEW_API_2_2_0 and ROYALE_FINAL_API_2_2_0 are no longer supported! (See CAPIVersion.h)
/*
 * These macros were used for migration from API versions before v2.2.0
 * to the v2.2.0 API. Support for APIs older than 2.2.0 has been removed in
 * Royale v3.0.0.
 *
 * If you have defined ROYALE_NEW_API_2_2_0 but not ROYALE_FINAL_API_2_2_0
 * this means the migration to the API introduced in v2.2.0 wasn't completed.
 * It's recommended to finish the migration to this API version first;
 * downgrading to a previous Royale release which still supports these macros
 * (e.g. Royale v2.3.0) may help with this.
 *
 * If you have defined both ROYALE_NEW_API_2_2_0 and ROYALE_FINAL_API_2_2_0,
 * you can check whether migration is necessary by compiling your sources with the
 * latest C API version (just leave ROYALE_NEW_API_2_2_0, ROYALE_FINAL_API_2_2_0
 * as well as ROYALE_C_API_VERSION unset).
 * If there are errors or warning regarding the API functions, define
 * ROYALE_C_API_VERSION=220 and follow the migration strategy detailed in the comment
 * about ROYALE_C_API_VERSION above.
 *
 */
#endif

#if ROYALE_C_API_VERSION == 31000
#include <CAPIVersion31000.h>
#elif ROYALE_C_API_VERSION == 330
#include <CAPIVersion330.h>
#elif ROYALE_C_API_VERSION == 320
#include <CAPIVersion320.h>
#elif ROYALE_C_API_VERSION == 300
#include <CAPIVersion300.h>
#elif ROYALE_C_API_VERSION == 220
#include <CAPIVersion220.h>
#else
#error Unsupported C API version!
#endif
#if defined(ROYALE_C_API_MIGRATION)
#include <CAPIMigration.h>
#endif

/** @}*/
