#ifndef _KROTOS_VOCODER_H_
#define _KROTOS_VOCODER_H_

#include "KrotosVocoderFXParams.h"
#include "KrotosVocoderFXDSP.h"

//-----------------------------------------------------------------------------
// Name: class CKrotosVocoderFX
//-----------------------------------------------------------------------------
class CKrotosVocoderFX : public AK::IAkInPlaceEffectPlugin
{
public:

    /// Constructor
    CKrotosVocoderFX();

     /// Destructor
    ~CKrotosVocoderFX();

    /// Effect plug-in initialization
    AKRESULT Init(AK::IAkPluginMemAlloc*        in_pAllocator,      /// Memory allocator interface.
                  AK::IAkEffectPluginContext*   in_pFXCtx,          /// Sound engine plug-in execution context.
                  AK::IAkPluginParam*           in_pParams,         /// Associated effect parameters node.
                  AkAudioFormat&                in_rFormat          /// Input/output audio format.
                  );
    
    /// Effect plug-in termination
    AKRESULT Term( AK::IAkPluginMemAlloc* in_pAllocator );

    /// Reset effect
    AKRESULT Reset( );

    /// Effect info query
    AKRESULT GetPluginInfo( AkPluginInfo& out_rPluginInfo );

    /// Effect plug-in DSP processing
    void Execute(AkAudioBuffer* io_pBuffer); /// Input/Output audio buffer structure.        

    /// Execution processing when the voice is virtual. Nothing special to do for this effect.
    AKRESULT TimeSkip(AkUInt32 in_uFrames){ return AK_DataReady; }

private:

    /// Cached information
    CKrotosVocoderFXDSP*     m_FXState;      /// Internal effect state
    CKrotosVocoderFXParams* m_pParams;      /// Effect parameter node
    AK::IAkPluginMemAlloc*  m_pAllocator;   /// Memory allocator interface
};

#endif // _KROTOS_VOCODER_H_
