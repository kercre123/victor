/**
File: lucasKanade_General.cpp
Author: Peter Barnum
Created: 2014-03-18

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/lucasKanade.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/interpolate.h"
#include "anki/common/robot/arrayPatterns.h"
#include "anki/common/robot/find.h"
#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/draw.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/errorHandling.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
     
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
