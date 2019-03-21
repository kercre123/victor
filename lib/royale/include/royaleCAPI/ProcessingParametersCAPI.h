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

#include <VariantCAPI.h>
#include <DefinitionsCAPI.h>
#include <DataStructuresCAPI.h>
#include <stdint.h>

ROYALE_CAPI_LINKAGE_TOP

typedef enum royale_processing_flag
{
    ROYALE_PROC_FLAG_CONSISTENCY_TOLERANCE_FLOAT = 0,     ///< Consistency limit for asymmetry validation
    ROYALE_PROC_FLAG_FLYING_PIXELSF0_FLOAT,               ///< Scaling factor for lower depth value normalization
    ROYALE_PROC_FLAG_FLYING_PIXELSF1_FLOAT,               ///< Scaling factor for upper depth value normalization
    ROYALE_PROC_FLAG_FLYING_PIXELS_FAR_DIST_FLOAT,        ///< Upper normalized threshold value for flying pixel detection
    ROYALE_PROC_FLAG_FLYING_PIXELS_NEAR_DIST_FLOAT,       ///< Lower normalized threshold value for flying pixel detection
    ROYALE_PROC_FLAG_LOWER_SATURATION_THRESHOLD_INT,      ///< Lower limit for valid raw data values
    ROYALE_PROC_FLAG_UPPER_SATURATION_THRESHOLD_INT,      ///< Upper limit for valid raw data values
    ROYALE_PROC_FLAG_MPI_AMP_THRESHOLD_FLOAT,             ///< Threshold for MPI flags triggered by amplitude discrepancy
    ROYALE_PROC_FLAG_MPI_DIST_THRESHOLD_FLOAT,            ///< Threshold for MPI flags triggered by distance discrepancy
    ROYALE_PROC_FLAG_MPI_NOISE_DISTANCE_FLOAT,            ///< Threshold for MPI flags triggered by noise
    ROYALE_PROC_FLAG_NOISE_THRESHOLD_FLOAT,               ///< Upper threshold for final distance noise
    ROYALE_PROC_FLAG_ADAPTIVE_NOISE_FILTER_TYPE_INT,      ///< Kernel type of the adaptive noise filter
    ROYALE_PROC_FLAG_AUTO_EXPOSURE_REF_AMPLITUDE_FLOAT,   ///< DEPRECATED : The reference amplitude for the new exposure estimate
    ROYALE_PROC_FLAG_USE_ADAPTIVE_NOISE_FILTER_BOOL,      ///< Activate spatial filter reducing the distance noise
    ROYALE_PROC_FLAG_USE_AUTO_EXPOSURE_BOOL,              ///< DEPRECATED : Activate dynamic control of the exposure time
    ROYALE_PROC_FLAG_USE_REMOVE_FLYING_PIXEL_BOOL,        ///< Activate FlyingPixel flag
    ROYALE_PROC_FLAG_USEMPI_FLAG_AVERAGE_BOOL,            ///< Activate spatial averaging MPI value before thresholding
    ROYALE_PROC_FLAG_USEMPI_FLAG_AMP_BOOL,                ///< Activates MPI-amplitude flag
    ROYALE_PROC_FLAG_USEMPI_FLAG_DIST_BOOL,               ///< Activates MPI-distance flag
    ROYALE_PROC_FLAG_USE_VALIDATE_IMAGE_BOOL,             ///< Activates output image validation
    ROYALE_PROC_FLAG_USE_REMOVE_STRAYLIGHT_BOOL,          ///< Activates the removal of stray light
    ROYALE_PROC_FLAG_USE_SPARSE_POINT_CLOUD_BOOL,         ///< DEPRECATED : Creates a sparse-point cloud in Spectre
    ROYALE_PROC_FLAG_USE_FILTER_TWO_FREQ_BOOL,            ///< Activates 2 frequency filtering
    ROYALE_PROC_FLAG_GLOBAL_BINNING_INT,                  ///< Size of the global binning kernel
    ROYALE_PROC_FLAG_USE_ADAPTIVE_BINNING_BOOL,           ///< DEPRECATED : Activate the adaptive binning
    ROYALE_PROC_FLAG_AUTO_EXPOSURE_REF_VALUE_FLOAT,       ///< The reference value for the new exposure estimate
    ROYALE_PROC_FLAG_USE_SMOOTHING_FILTER_BOOL,           ///< Enable/Disable the smoothing filter
    ROYALE_PROC_FLAG_SMOOTHING_ALPHA_FLOAT,               ///< The alpha value used for the smoothing filter
    ROYALE_PROC_FLAG_SMOOTHING_FILTER_TYPE_INT,           ///< Determines the type of smoothing that is used
    ROYALE_PROC_FLAG_USE_FLAG_SBI_BOOL,                   ///< Enable/Disable the flagging of pixels where the SBI was active
    ROYALE_PROC_FLAG_USE_HOLE_FILLING_BOOL,               ///< Enable/Disable the hole filling algorithm
    ROYALE_PROC_FLAG_NUM_FLAGS
} royale_processing_flag;

/**
 * see royale::getProcessingFlagName()
 * use ::royale_free_string_array() to free memory of the returned array.
 */
ROYALE_CAPI void royale_proc_flag_get_flag_name (char **name_dst, royale_processing_flag flag);

typedef struct
{
    royale_processing_flag flag;
    royale_variant value;
} royale_processing_parameter;

ROYALE_CAPI_LINKAGE_BOTTOM
/** @}*/
