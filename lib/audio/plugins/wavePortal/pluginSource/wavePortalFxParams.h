/*
 * File: wavePortalFxParams.h
 *
 * Author: Jordan Rivas
 * Created: 3/24/2016
 *
 * Description: This class has WWise hooks to get and set WavePortal Plug-in parameters.
 *
 * Copyright: Anki, Inc. 2016
 */

#ifndef __AnkiAudio_PlugIns_WavePortalFxParams_H__
#define __AnkiAudio_PlugIns_WavePortalFxParams_H__

#include "AK/SoundEngine/Common/IAkPlugin.h"


namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  
static const AkPluginParamID AK_PLUGIN_ID	= 0;

class WavePortalFxParams : public AK::IAkPluginParam
{
public:
  
  WavePortalFxParams();
  
  // Create paramater node duplicate
  AK::IAkPluginParam* Clone( AK::IAkPluginMemAlloc* in_pAllocator ) override;
  
  // Parameter node initialization
  AKRESULT Init( AK::IAkPluginMemAlloc*	in_pAllocator,
                 const void* in_pParamsBlock,
                 AkUInt32	in_ulBlockSize ) override;
  
  // Parameter node termination
  AKRESULT Term( AK::IAkPluginMemAlloc* in_pAllocator ) override;
  
  // Parameter block set
  AKRESULT SetParamsBlock( const void* in_pParamsBlock,
                           AkUInt32	in_ulBlockSize ) override;
  
  // Update a single parameter
  AKRESULT SetParam( AkPluginParamID in_ParamID,
                     const void* in_pValue,
                     AkUInt32	in_ulParamSize ) override;
  
  AkInt32 GetPlugInId() const { return _plugInId; }
  
private:
  
  AkInt32 _plugInId = 0;
  
};


} // PlugIns
} // AudioEngine
} // Anki

#endif /* __AnkiAudio_PlugIns_WavePortalFxParams_H__ */
