/**
 * File: linearClassifier.h
 * 
 * Author: Humphrey Hu
 * Date:   2018-05-25
 * 
 * Description: Linear classifier execution class
 * 
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/matrix.h"
#include "json/json-forwards.h"

#include <vector>

#ifndef __Anki_Coretech_Common_Math_LinearClassifier_H__
#define __Anki_Coretech_Common_Math_LinearClassifier_H__

namespace Anki {

/**
 * Log-odds linear classifier
 * 
 * Computes the log ratio of positive to negative classes as a linear (dot) product with a bias term. 
 * Uses single precision floating point and provides methods for interpreting output in various ways.
 * 
 * Parameters are loaded through JSON, typically as a subfield from the owning module or class's config.
 * 
 * Classify methods are templatized to operate on any []-iterable object, and return NaN when
 * the input size is incorrect.
 * TODO Extend this to other generic types, possibly switch to an actual linear algebra library
 * 
 * Note that there are template implementations in linearClassifier_impl.h which must be included in your
 * source files for compilation.
 */

class LinearClassifier 
{
public:

  LinearClassifier();
  Result Init( const Json::Value& config );
  
  // Returns the ratio of probabilities (odds) of positive over negative class
  template <typename T>
  f32 ClassifyOdds( const T& x ) const;
  
  // Returns the raw output from the linear classifier, the log odds
  template <typename T>  
  f32 ClassifyLogOdds( const T& x ) const;

  // Returns the probability of the positive class
  template <typename T>
  f32 ClassifyProbability( const T& x ) const;

  size_t GetInputDim() const;

private:

  static constexpr f32 kMaxOdds = 1e3f;
  static constexpr f32 kMinOdds = 1e-3f;

  bool _isInitialized;
  std::vector<f32> _weights;
  f32 _offset;

  // Checks to make sure an input has the right dimensionality
  template <typename T>
  bool CheckInputDim( const T& x ) const;
  
  // Checks to make sure the classifier is initialized
  bool CheckInit() const;
};

} // end namespace Anki

#endif // __Anki_Coretech_Common_Math_LinearClassifier_H__