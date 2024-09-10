/*
 * File: HijackFxParams.cpp
 *
 * Author: Jordan Rivas
 * Created: 11/10/2015
 *
 * Description: This class has WWise hooks to get and set Hijack Plug-in parameters.
 *        Note: We are not currently using any parameters for Hijack Plug-in
 *
 * Copyright: Anki, Inc. 2015
 */


#include "hijackFxParams.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HijackFxParams::HijackFxParams()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AK::IAkPluginParam* HijackFxParams::Clone( AK::IAkPluginMemAlloc* in_pAllocator )
{
  return AK_PLUGIN_NEW( in_pAllocator, HijackFxParams( *this ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFxParams::Init( AK::IAkPluginMemAlloc*	in_pAllocator,
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
AKRESULT HijackFxParams::Term( AK::IAkPluginMemAlloc* in_pAllocator )
{
  AK_PLUGIN_DELETE( in_pAllocator, this );
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFxParams::SetParamsBlock( const void* in_pParamsBlock,
                                        AkUInt32	in_ulBlockSize )
{
  AKRESULT result = AK_Success;
  AkUInt8 * pParamsBlock = (AkUInt8 *)in_pParamsBlock;
  _plugInId = READBANKDATA( AkInt32, pParamsBlock, in_ulBlockSize );
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AKRESULT HijackFxParams::SetParam( AkPluginParamID in_ParamID,
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
