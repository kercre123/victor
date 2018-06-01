/**
 * File: linearClassifier.cpp
 * 
 * Author: Humphrey Hu
 * Date:   2018-05-25
 * 
 * Description: Linear classifier execution class
 * 
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/linearClassifier.h"

#include <cmath>

namespace Anki {

LinearClassifier::LinearClassifier()
: _isInitialized( false )
, _offset( 0.0f )
{}

Result LinearClassifier::Init( const Json::Value& config )
{
  // Parse parameters from JSON file
  // TODO: Check for infs, nans, etc?
  _isInitialized = false;
  if( !JsonTools::GetVectorOptional<f32>(config, "Weights", _weights) )
  {
    PRINT_NAMED_ERROR( "LinearClassifier.Init.MissingParameter", 
                       "Missing Weights parameter" );
    return RESULT_FAIL;
  }
  if( !JsonTools::GetValueOptional<f32>( config, "Offset", _offset ) )
  {
    PRINT_NAMED_ERROR( "LinearClassifier.Init.MissingParameter",
                       "Missing Offset parameter" );
    return RESULT_FAIL;
  }

  _isInitialized = true;
  return RESULT_OK;
}

size_t LinearClassifier::GetInputDim() const
{
  if( !CheckInit() )
  {
    return 0;
  }
  return _weights.size();
}

bool LinearClassifier::CheckInit() const
{
  if( !_isInitialized )
  {
    PRINT_NAMED_ERROR( "LinearClassifier.CheckInit.Uninitialized", "" );
  }
  return _isInitialized;
}

} // end namespace Anki