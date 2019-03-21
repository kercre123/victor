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

#include <CAPIVersion.h>
#include <DefinitionsCAPI.h>
#include <StatusCAPI.h>

ROYALE_CAPI_LINKAGE_TOP


/**
 * see royale::getVersion()
 * Get the royale library version.
 *
 * @param[in,out] major Pointer to an unsigned where the major version should be written to.
 * @param[in,out] minor Pointer to an unsigned where the minor version should be written to.
 * @param[in,out] patch Pointer to an unsigned where the patch level should be written to.
 *
 * Note that all pointers must point to valid memory locations (passing NULL is not allowed).
 */
ROYALE_CAPI void royale_get_version_v220 (unsigned *major, unsigned *minor, unsigned *patch);
/**
 * see royale::getVersion()
 * Get the royale library version and build number.
 *
 * @param[in,out] major Pointer to an unsigned where the major version should be written to.
 * @param[in,out] minor Pointer to an unsigned where the minor version should be written to.
 * @param[in,out] patch Pointer to an unsigned where the patch level should be written to.
 * @param[in,out] build Pointer to an unsigned where the build number should be written to.
 *
 * Note that all pointers must point to valid memory locations (passing NULL is not allowed).
 */
ROYALE_CAPI void royale_get_version_with_build_v220 (unsigned *major, unsigned *minor, unsigned *patch, unsigned *build);
/**
 * see royale::getVersion()
 * Get the royale library version, build number and SCM revision.
 *
 * @param[in,out] major Pointer to an unsigned where the major version should be written to.
 * @param[in,out] minor Pointer to an unsigned where the minor version should be written to.
 * @param[in,out] patch Pointer to an unsigned where the patch level should be written to.
 * @param[in,out] build Pointer to an unsigned where the build number should be written to.
 * @param[in,out] scm_rev Pointer where the SCM revision should be written to.
 * royale_free_string() has to be used on the string returned in scm_rev.
 * @return ROYALE_STATUS_SUCCESS on success, or an error code if allocating memory for scm_ref fails.
 *
 * Note that all pointers must point to valid memory locations (passing NULL is not allowed).
 */
ROYALE_CAPI royale_camera_status royale_get_version_with_build_and_scm_revision_v320 (unsigned *major, unsigned *minor, unsigned *patch, unsigned *build, char **scm_rev);

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
