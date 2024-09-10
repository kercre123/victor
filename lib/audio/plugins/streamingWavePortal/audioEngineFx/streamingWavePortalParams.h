/*
 * File: streamingWavePortalParams.h
 *
 * Author: Jordan Rivas
 * Created: 07/17/2018
 *
 * Description: StreamingWavePortal Plug-in common header to hook the plug-in into Wwise’s plug-in create & set
 *              parameters function pointers.
 *
 * Copyright: Anki, Inc. 2018
 */


#ifndef __AnkiAudio_Plugins_StreamingWavePortalParams_H__
#define __AnkiAudio_Plugins_StreamingWavePortalParams_H__

#include <AK/Tools/Common/AkAssert.h>
#include <AK/Plugin/PluginServices/AkFXParameterChangeHandler.h>
#include <math.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>

namespace Anki {
namespace AudioEngine {
namespace PlugIns {
  
// Parameters IDs.
static const AkPluginParamID  SWP_FXPARAM_INSTANCE_ID = 0;
static const AkUInt32         SWP_NUMFXPARAMS         = 1;


// Parameters struture for this effect.
struct StreamingWavePortalFXParams
{
  AkInt32 instanceId;   // Id of plugin instance
} AK_ALIGN_DMA;

class StreamingWavePortalParams : public AK::IAkPluginParam, public StreamingWavePortalFXParams
{
public:

  // Constructor.
  StreamingWavePortalParams();

  // Copy constructor.
  StreamingWavePortalParams( const StreamingWavePortalParams & in_rCopy );

  // Destructor.
  virtual ~StreamingWavePortalParams();

  // Create parameter object duplicate.
  virtual AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc* in_pAllocator );

  // Initialization.
  virtual AKRESULT Init( AK::IAkPluginMemAlloc*   in_pAllocator,		// Memory allocator.
                         const void*              in_pParamsBlock,	// Pointer to param block.
                         AkUInt32                 in_ulBlockSize );	// Sise of the parm block.

  // Termination.
  virtual AKRESULT Term( AK::IAkPluginMemAlloc* in_pAllocator );

  // Set all parameters at once.
  virtual AKRESULT SetParamsBlock( const void*  in_pParamsBlock,
                                   AkUInt32     in_ulBlockSize );

  // Update one parameter.
  virtual AKRESULT SetParam( AkPluginParamID  in_ParamID,
                             const void*      in_pValue,
                             AkUInt32         in_ulParamSize );


public:

  AK::AkFXParameterChangeHandler<SWP_NUMFXPARAMS> m_ParamChangeHandler;
};


} // Plugins
} // AudioEngine
} // Anki

#endif // __AnkiAudio_Plugins_StreamingWavePortalParams_H__
