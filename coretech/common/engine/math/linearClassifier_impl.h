/**
 * File: linearClassifier_impl.h
 * 
 * Author: Humphrey Hu
 * Date:   2018-05-30
 * 
 * Description: Linear classifier execution class
 * 
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/engine/math/linearClassifier.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {

template <typename T>
f32 LinearClassifier::ClassifyOdds( const T& x ) const
{
  // NOTE NaN will propagate through std::exp
  return std::exp( ClassifyLogOdds<T>( x ) );
}

template <typename T>
f32 LinearClassifier::ClassifyLogOdds( const T& x ) const
{
  if( !CheckInputDim( x ) || !CheckInit() )
  {
    return std::numeric_limits<f32>::quiet_NaN();
  }
  
  // TODO: Check for infs, nans, etc?
  f32 acc = _offset;
  for( int i = 0; i < _weights.size(); ++i )
  {
    acc += x[i] * _weights[i];
  }
  return acc;
}

template <typename T>
f32 LinearClassifier::ClassifyProbability( const T& x ) const
{
  // Odds are ratio of positive/negative probabilities
  f32 odds = ClassifyOdds<T>( x );

  // NOTE: Dangerous to compute the logistic directly since we may get infs
  // NOTE: NaN should fail both comparisons and propagate through
  if( Util::IsFltGT(odds, kMaxOdds ) )
  {
    return 1.0f;
  }
  else if( Util::IsFltLT(odds, kMinOdds ) )
  {
    return 0.0f;
  }
  else
  {
    return odds / (1.0f + odds);
  }
}

template <typename T>
bool LinearClassifier::CheckInputDim( const T& x ) const
{
  if( x.size() != _weights.size() )
  {
    PRINT_NAMED_ERROR("LinearClassifier.CheckInputDim.WrongInputDim",
                      "Got %u but expected %u values", (int) x.size(), (int) _weights.size());
    return false;
  }
  return true;
}

} // end namespace Anki