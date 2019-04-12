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
#include "util/helpers/templateHelpers.h"
 
namespace Anki {
namespace Vision {
  
  TrackedFace::TrackedFace()
  : _smileAmount(false, 0.f, 0.f)
  , _gaze(false, 0.f, 0.f)
  , _blinkAmount(false, 0.f, 0.f)
  , _headPose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
  , _eyePose(M_PI_2, X_AXIS_3D(), {0.f, 0.f, 0.f})
  {
    
  }

  const TrackedFace::ExpressionValues& TrackedFace::GetExpressionValues() const
  {
    if(ANKI_DEVELOPER_CODE)
    {
      // Documentation (in trackedFace.h and in Okao expression manual) says this should be a histogram
      // summing to 100. Verify that's true (in debug builds).
      s32 sum=0;
      for(auto const& value : _expression)
      {
        sum += value;
      }
      const bool noExpressionValues = (sum==0);
      DEV_ASSERT_MSG(noExpressionValues || (sum==100), "TrackedFace.GetExpressionValues.HistSumNot100", "%d", sum);
    }
    
    return _expression;
  }
  
  // Return the expression with highest value
  FacialExpression TrackedFace::GetMaxExpression(ExpressionValue* valuePtr) const
  {
    static_assert(Util::EnumToUnderlying(FacialExpression::Unknown) == -1,
                  "Expecting Unknown expression to be value -1");
    
    // This tracks the index of the expression in the array with the max value
    auto maxExprIndex = Util::EnumToUnderlying(FacialExpression::Unknown);
    
    // This is used to increment through the expression array, starting from the 0 index,
    // and using the underlying type of the FacialExpression enum
    auto crntExpIndex = static_cast<std::underlying_type_t<FacialExpression>>(0);
    
    ExpressionValue maxValue = 0;
    for(auto const& crntValue : _expression)
    {
      if(crntValue > maxValue) {
        maxValue = crntValue;
        maxExprIndex = crntExpIndex;
      }
      ++crntExpIndex;
    }
    
    if(nullptr != valuePtr)
    {
      *valuePtr = maxValue;
    }
    return static_cast<FacialExpression>(maxExprIndex);
  }
  
  void TrackedFace::SetExpressionValue(FacialExpression whichExpression, ExpressionValue newValue)
  {
    DEV_ASSERT_MSG(Util::InRange(newValue, ExpressionValue(0), ExpressionValue(100)),
                   "TrackedFace.SetExpressionValue.BadValue",
                   "%s:%d", EnumToString(whichExpression), newValue);
    
    const auto expressionIndex = Util::EnumToUnderlying(whichExpression);
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


