#include "KrotosVocoderFX.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/AkWwiseSDKVersion.h>

/// Plugin mechanism. Instantiation method that must be registered to the plug-in manager.
AK::IAkPlugin* CreateKrotosVocoderFX( AK::IAkPluginMemAlloc* in_pAllocator )
{
	//AKPLATFORM::OutputDebugMsg("Create vocoder\n");
    return AK_PLUGIN_NEW( in_pAllocator, CKrotosVocoderFX( ) );
}

/// Plugin mechanism. Instantiation method that must be registered to the plug-in manager.
AK::IAkPluginParam* CreateKrotosVocoderFXParams(AK::IAkPluginMemAlloc* in_pAllocator)
{
	//AKPLATFORM::OutputDebugMsg("Create vocoder params\n");
    return AK_PLUGIN_NEW(in_pAllocator, CKrotosVocoderFXParams());
}

#ifndef AKCOMPANYID_KROTOS
#define AKCOMPANYID_KROTOS 269
#endif

AK_IMPLEMENT_PLUGIN_FACTORY(KrotosVocoderFX, AkPluginTypeEffect, AKCOMPANYID_KROTOS, 42)

/// Constructor.
CKrotosVocoderFX::CKrotosVocoderFX()
    : m_FXState(NULL)
	, m_pParams( NULL )
    , m_pAllocator( NULL )
{
}

/// Destructor.
CKrotosVocoderFX::~CKrotosVocoderFX()
{
}

/// Initializes and allocate memory for the effect.
AKRESULT CKrotosVocoderFX::Init(AK::IAkPluginMemAlloc*      in_pAllocator,      /// Memory allocator interface.
                            AK::IAkEffectPluginContext*     in_pFXCtx,          /// Sound engine plug-in execution context.
                            AK::IAkPluginParam*             in_pParams,         /// Associated effect parameters node.
                            AkAudioFormat&                  in_rFormat          /// Input/output audio format.
                            )
{
	m_pParams = (CKrotosVocoderFXParams*)in_pParams;
    m_pAllocator = in_pAllocator;

	//AKPLATFORM::OutputDebugMsg("init allocating\n");
	m_FXState = AK_PLUGIN_NEW(m_pAllocator, CKrotosVocoderFXDSP());

	if (m_FXState)
	{
		//AKPLATFORM::OutputDebugMsg("Setup \n");
		m_FXState->Setup(m_pParams, in_pFXCtx->IsSendModeEffect(), in_rFormat.uSampleRate, m_pAllocator);
	}
	else
	{
		return AK_Fail;
	}
 
    //m_FXState.Setup( m_pParams, in_pFXCtx->IsSendModeEffect(), in_rFormat.uSampleRate );

    AK_PERF_RECORDING_RESET();

    return AK_Success;
}

/// Effect termination.
AKRESULT CKrotosVocoderFX::Term( AK::IAkPluginMemAlloc* in_pAllocator )
{
	if (m_FXState)
	{
		AK_PLUGIN_DELETE(m_pAllocator, m_FXState);
		m_FXState = NULL;
	}

    AK_PLUGIN_DELETE( in_pAllocator, this ); /// Effect must delete itself
    return AK_Success;
}

/// Actions to perform on FX reset (example on bypass)
AKRESULT CKrotosVocoderFX::Reset( )
{
	//AKPLATFORM::OutputDebugMsg("vocoder reset\n");
    return AK_Success;
}

/// Effect info query.
AKRESULT CKrotosVocoderFX::GetPluginInfo( AkPluginInfo& out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeEffect;
    out_rPluginInfo.bIsInPlace = true;

    out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
#ifndef WWISE_2017
//	out_rPluginInfo.bIsAsynchronous = false;
#endif
    return AK_Success;
}

/// Effect plug-in DSP processing
void CKrotosVocoderFX::Execute(AkAudioBuffer* io_pBuffer) /// Input/Output audio buffer structure.
{
    AK_PERF_RECORDING_START( "KrotosVocoder", 25, 30 );
    // Execute DSP processing synchronously here
    m_FXState->Process( io_pBuffer, m_pParams );
    AK_PERF_RECORDING_STOP( "KrotosVocoder", 25, 30 );
}
