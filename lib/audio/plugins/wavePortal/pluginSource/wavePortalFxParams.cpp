/*
 * File: wavePortalFxParams.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: WavePortal Plug-in common header to hook the plug-in into Wwise’s plug-in create & set parameters
 *              function pointers.
 *
 * Copyright: Anki, Inc. 2015
 */


#include "wavePortalFxParams.h"
#include "AK/Tools/Common/AkBankReadHelpers.h"


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WavePortalFxParams::WavePortalFxParams()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AK::IAkPluginParam* WavePortalFxParams::Clone( AK::IAkPluginMemAlloc* in_pAllocator )
{
  return AK_PLUGIN_NEW( in_pAllocator, WavePortalFxParams( *this ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFxParams::Init( AK::IAkPluginMemAlloc*	in_pAllocator,
                              const void* in_pParamsBlock,
                              AkUInt32	in_ulBlockSize )
{
  if ( in_ulBlockSize == 0)
  {
    // TODO: Init default parameters.

    return AK_Success;
  }
  return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFxParams::Term( AK::IAkPluginMemAlloc* in_pAllocator )
{
  AK_PLUGIN_DELETE( in_pAllocator, this );
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFxParams::SetParamsBlock( const void* in_pParamsBlock,
                                        AkUInt32	in_ulBlockSize )
{
  AKRESULT result = AK_Success;
  AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
  _plugInId = READBANKDATA( AkInt32, pParamsBlock, in_ulBlockSize );
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT WavePortalFxParams::SetParam( AkPluginParamID in_ParamID,
                                  const void* in_pValue,
                                  AkUInt32	in_ulParamSize )
{
  AKRESULT result = AK_Success;
  // This shouldn't happen we set the id in Wwise editor and don't change it during run time
  switch ( in_ParamID ) {
    case AK_PLUGIN_ID:
      _plugInId = *(AkInt32*)(in_pValue);
      break;
      
    default:
      break;
  }

  return result;
}


} // PlugIns
} // AudioEngine
} // Anki
