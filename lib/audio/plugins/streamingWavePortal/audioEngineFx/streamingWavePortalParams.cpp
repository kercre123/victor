/*
 * File: streamingWavePortalParams.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/17/2018
 *
 * Description: StreamingWavePortal Plug-in common header to hook the plug-in into Wwise’s plug-in create & set
 *              parameters function pointers.
 *
 * Copyright: Anki, Inc. 2018
 */


#include "streamingWavePortalParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// Constructor.
StreamingWavePortalParams::StreamingWavePortalParams()
{
}

// Destructor.
StreamingWavePortalParams::~StreamingWavePortalParams()
{
}

// Copy constructor.
StreamingWavePortalParams::StreamingWavePortalParams( const StreamingWavePortalParams & in_rCopy )
{
  *static_cast<StreamingWavePortalFXParams*>(this) = static_cast<StreamingWavePortalFXParams>(in_rCopy);
  m_ParamChangeHandler.SetAllParamChanges();
}

// Create parameter node duplicate.
AK::IAkPluginParam * StreamingWavePortalParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
  return AK_PLUGIN_NEW( in_pAllocator, StreamingWavePortalParams(*this) );
}

// Parameter node initialization.
AKRESULT StreamingWavePortalParams::Init( AK::IAkPluginMemAlloc*  in_pAllocator,
                                          const void*             in_pParamsBlock,
                                          AkUInt32                in_ulBlockSize )
{
  if ( in_ulBlockSize == 0)
  {
    // Init default parameters.
    instanceId = 0;
    m_ParamChangeHandler.SetAllParamChanges();
    return AK_Success;
  }
  return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Parameter node termination.
AKRESULT StreamingWavePortalParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
  AK_PLUGIN_DELETE( in_pAllocator, this );
  return AK_Success;
}

// Set all shared parameters at once.
AKRESULT StreamingWavePortalParams::SetParamsBlock( const void* in_pParamsBlock,
                                                    AkUInt32 in_ulBlockSize )
{
  AKRESULT eResult = AK_Success;
  AkUInt8* pParamsBlock = (AkUInt8*)in_pParamsBlock;
  instanceId = READBANKDATA( AkInt32, pParamsBlock, in_ulBlockSize );
  CHECKBANKDATASIZE( in_ulBlockSize, eResult );
  m_ParamChangeHandler.SetAllParamChanges();
  return eResult;
}

// Update single parameter.
AKRESULT StreamingWavePortalParams::SetParam( AkPluginParamID in_ParamID,
                                              const void*     in_pValue,
                                              AkUInt32        in_ulParamSize )
{
  // Set parameter value.
  switch ( in_ParamID )
  {
    case SWP_FXPARAM_INSTANCE_ID:
    {
      instanceId = *(AkUInt8*)in_pValue;
      m_ParamChangeHandler.SetParamChange( SWP_FXPARAM_INSTANCE_ID );
      break;
    }
      
    default:
      return AK_InvalidParameter;
  }
  
  return AK_Success;
}


} // Plugins
} // AudioEngine
} // Anki
