/****************************************************************************\
 * Copyright (C) 2015 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <royale/Variant.hpp>
#include <royale/Vector.hpp>
#include <royale/String.hpp>

namespace royale
{
    /*!
     *  This is a list of flags which can be set/altered in access LEVEL 2 in order
     *  to control the processing pipeline. The suffix type indicates the expected Variant type.
     *  For a more detailed description of the different parameters please refer to the documentation
     *  you will receive after getting LEVEL 2 access.
     *
     *  Keep in mind, that if this list is changed, the map with names has to be adapted!
     */
    enum class ProcessingFlag
    {
        ConsistencyTolerance_Float = 0,     ///< Consistency limit for asymmetry validation
        FlyingPixelsF0_Float,               ///< Scaling factor for lower depth value normalization
        FlyingPixelsF1_Float,               ///< Scaling factor for upper depth value normalization
        FlyingPixelsFarDist_Float,          ///< Upper normalized threshold value for flying pixel detection
        FlyingPixelsNearDist_Float,         ///< Lower normalized threshold value for flying pixel detection
        LowerSaturationThreshold_Int,       ///< Lower limit for valid raw data values
        UpperSaturationThreshold_Int,       ///< Upper limit for valid raw data values
        MPIAmpThreshold_Float,              ///< Threshold for MPI flags triggered by amplitude discrepancy
        MPIDistThreshold_Float,             ///< Threshold for MPI flags triggered by distance discrepancy
        MPINoiseDistance_Float,             ///< Threshold for MPI flags triggered by noise
        NoiseThreshold_Float,               ///< Upper threshold for final distance noise
        AdaptiveNoiseFilterType_Int,        ///< Kernel type of the adaptive noise filter
        AutoExposureRefAmplitude_Float,     ///< DEPRECATED : The reference amplitude for the new exposure estimate
        UseAdaptiveNoiseFilter_Bool,        ///< Activate spatial filter reducing the distance noise
        UseAutoExposure_Bool,               ///< DEPRECATED : Activate dynamic control of the exposure time
        UseRemoveFlyingPixel_Bool,          ///< Activate FlyingPixel flag
        UseMPIFlagAverage_Bool,             ///< Activate spatial averaging MPI value before thresholding
        UseMPIFlag_Amp_Bool,                ///< Activates MPI-amplitude flag
        UseMPIFlag_Dist_Bool,               ///< Activates MPI-distance flag
        UseValidateImage_Bool,              ///< Activates output image validation
        UseRemoveStrayLight_Bool,           ///< Activates the removal of stray light
        UseSparsePointCloud_Bool,           ///< DEPRECATED : Creates a sparse-point cloud in Spectre
        UseFilter2Freq_Bool,                ///< Activates 2 frequency filtering
        GlobalBinning_Int,                  ///< Sets the size of the global binning kernel
        UseAdaptiveBinning_Bool,            ///< DEPRECATED : Activates adaptive binning
        AutoExposureRefValue_Float,         ///< The reference value for the new exposure estimate
        UseSmoothingFilter_Bool,            ///< Enable/Disable the smoothing filter
        SmoothingAlpha_Float,               ///< The alpha value used for the smoothing filter
        SmoothingFilterType_Int,            ///< Determines the type of smoothing that is used
        UseFlagSBI_Bool,                    ///< Enable/Disable the flagging of pixels where the SBI was active
        UseHoleFilling_Bool,                ///< Enable/Disable the hole filling algorithm
        NUM_FLAGS
    };

    /*!
    * For debugging, printable strings corresponding to the ProcessingFlag enumeration. The
    * returned value is copy of the processing flag name. If the processing flag is not found
    * an empty string will be returned.
    *
    * These strings will not be localized.
    */
    ROYALE_API royale::String getProcessingFlagName (royale::ProcessingFlag mode);

    /*!
    * Convert a string received from getProcessingFlagName back into its ProcessingFlag.
    * If the processing flag name is not found the method returns false.
    * Else the method will return true.
    */
    ROYALE_API bool parseProcessingFlagName (const royale::String &modeName, royale::ProcessingFlag &processingFlag);

    /*!
    *  This is a map combining a set of flags which can be set/altered in access LEVEL 2 and the set value as Variant type.
    *  The proposed minimum and maximum limits are recommendations for reasonable results. Values beyond these boundaries are
    *  permitted, but are currently neither evaluated nor verified.
    */
    typedef royale::Vector< royale::Pair<royale::ProcessingFlag, royale::Variant> > ProcessingParameterVector;
    typedef std::map< royale::ProcessingFlag, royale::Variant > ProcessingParameterMap;
    typedef std::pair< royale::ProcessingFlag, royale::Variant > ProcessingParameterPair;

#ifndef SWIG

    /**
     * Takes ProcessingParameterMaps a and b and returns a combination of both.
     * Keys that exist in both maps will take the value of map b.
     */
    ROYALE_API ProcessingParameterMap combineProcessingMaps (const ProcessingParameterMap &a,
            const ProcessingParameterMap  &b);

    namespace parameter
    {
        static const ProcessingParameterPair stdConsistencyTolerance ({ royale::ProcessingFlag::ConsistencyTolerance_Float, royale::Variant (1.2f, 0.2f, 1.5f) });
        static const ProcessingParameterPair stdFlyingPixelsF0 ({ royale::ProcessingFlag::FlyingPixelsF0_Float, royale::Variant (0.018f, 0.01f, 0.04f) });
        static const ProcessingParameterPair stdFlyingPixelsF1 ({ royale::ProcessingFlag::FlyingPixelsF1_Float, royale::Variant (0.14f, 0.01f, 0.5f) });
        static const ProcessingParameterPair stdFlyingPixelsFarDist ({ royale::ProcessingFlag::FlyingPixelsFarDist_Float, royale::Variant (4.5f, 0.01f, 1000.0f) });
        static const ProcessingParameterPair stdFlyingPixelsNearDist ({ royale::ProcessingFlag::FlyingPixelsNearDist_Float, royale::Variant (1.0f, 0.01f, 1000.0f) });
        static const ProcessingParameterPair stdLowerSaturationThreshold ({ royale::ProcessingFlag::LowerSaturationThreshold_Int, royale::Variant (400, 0, 600) });
        static const ProcessingParameterPair stdUpperSaturationThreshold ({ royale::ProcessingFlag::UpperSaturationThreshold_Int, royale::Variant (3750, 3500, 4095) });
        static const ProcessingParameterPair stdMPIAmpThreshold ({ royale::ProcessingFlag::MPIAmpThreshold_Float, royale::Variant (0.3f, 0.1f, 0.5f) });
        static const ProcessingParameterPair stdMPIDistThreshold ({ royale::ProcessingFlag::MPIDistThreshold_Float, royale::Variant (0.1f, 0.05f, 0.2f) });
        static const ProcessingParameterPair stdMPINoiseDistance ({ royale::ProcessingFlag::MPINoiseDistance_Float, royale::Variant (3.0f, 2.0f, 5.0f) });
        static const ProcessingParameterPair stdNoiseThreshold ({ royale::ProcessingFlag::NoiseThreshold_Float, royale::Variant (0.07f, 0.03f, 0.2f) });
        static const ProcessingParameterPair stdAdaptiveNoiseFilterType ({ royale::ProcessingFlag::AdaptiveNoiseFilterType_Int, royale::Variant (1, 1, 2) });
        static const ProcessingParameterPair stdAutoExposureRefValue ({ royale::ProcessingFlag::AutoExposureRefValue_Float, 1000.0f });
        static const ProcessingParameterPair stdUseRemoveStrayLight ({ royale::ProcessingFlag::UseRemoveStrayLight_Bool, false });
        static const ProcessingParameterPair stdUseFlagSBI ({ royale::ProcessingFlag::UseFlagSBI_Bool, false });
        static const ProcessingParameterPair stdUseSmoothingFilter ({ royale::ProcessingFlag::UseSmoothingFilter_Bool, false });
        static const ProcessingParameterPair stdSmoothingFilterAlpha ({ royale::ProcessingFlag::SmoothingAlpha_Float, royale::Variant (0.3f, 0.1f, 1.0f) });
        static const ProcessingParameterPair stdSmoothingFilterType ({ royale::ProcessingFlag::SmoothingFilterType_Int, royale::Variant (1, 0, 1) });
        static const ProcessingParameterPair stdUseHoleFilling ({ royale::ProcessingFlag::UseHoleFilling_Bool, false });
    }
#endif
}
