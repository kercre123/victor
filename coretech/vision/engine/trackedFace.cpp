/**
 * File: trackedFace.cpp
 *
 * Author: Andrew Stein
 * Date:   8/20/2015
 *
 * Description: A container for a tracked face and any features (e.g. eyes, mouth, ...)
 *              related to it.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "coretech/vision/engine/trackedFace.h"
#include "coretech/common/engine/math/point_impl.h"

namespace Anki {
namespace Vision {
  
  TrackedFace::TrackedFace()
  : _smileAmount(false, 0.f, 0.f)
  , _gaze(false, 0.f, 0.f)
  , _blinkAmount(false, 0.f, 0.f)
  , _headPose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
  {
    
  }

  const TrackedFace::FacialExpressionValues& TrackedFace::GetExpressionValues() const
  {
    return _expression;
  }
  
  // Return the expression with highest value
  FacialExpression TrackedFace::GetMaxExpression(s32* valuePtr) const
  {
    static_assert((s32)FacialExpression::Unknown == -1, "Expecting Unknown expression to be value -1");
    
    FacialExpression maxExpression = FacialExpression::Unknown;
    s32 maxValue = -1;
    for(s32 crntExpression = 0; crntExpression < (s32)FacialExpression::Count; ++crntExpression)
    {
      if(_expression[crntExpression] > maxValue) {
        maxValue = _expression[crntExpression];
        maxExpression = (FacialExpression)crntExpression;
      }
    }
    
    if(nullptr != valuePtr)
    {
      *valuePtr = maxValue;
    }
    return maxExpression;
  }
  
  void TrackedFace::SetExpressionValue(FacialExpression whichExpression, f32 newValue)
  {
    const u32 expressionIndex = (u32)whichExpression;
    DEV_ASSERT(expressionIndex < (u32)FacialExpression::Count, "TrackedFace.SetExpressionValue.BadExpression");
    _expression[expressionIndex] = newValue;
  }

  void TrackedFace::Shift(const Point2f& shift)
  {
    for (auto& feature: _features)
    {
      for (auto& point: feature) 
      {
        point += shift;
      }
    }
    _rect = Rectangle<f32>(_rect.GetX() + shift.x(), _rect.GetY() + shift.y(),
                           _rect.GetWidth(), _rect.GetHeight());
  }
} // namespace Vision
} // namespace Anki


