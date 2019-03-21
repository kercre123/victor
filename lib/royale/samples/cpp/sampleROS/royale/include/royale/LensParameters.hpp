/****************************************************************************\
 * Copyright (C) 2015 Infineon Technologies
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <royale/Pair.hpp>
#include <royale/Vector.hpp>

namespace royale
{
    /**
     *  This container stores the lens parameters from the camera module
     */
    struct LensParameters
    {
        royale::Pair<float, float>     principalPoint;       //!< cx/cy
        royale::Pair<float, float>     focalLength;          //!< fx/fy
        royale::Pair<float, float>     distortionTangential; //!< p1/p2
        royale::Vector<float>          distortionRadial;     //!< k1/k2/k3
    };
}
