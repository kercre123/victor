//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// The main automotive audio engine interface.

#pragma once

#include "AkAutoEngineExports.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/AkCallback.h>
#include <AK/Plugin/AkAudioInputPlugin.h>
#include "AkPluginsExtApi.h"

/// Audiokinetic namespace
namespace AK
{
	/// Audiokinetic automotive audio engine namespace
	/// \remarks The functions in this namespace are thread-safe, unless stated otherwise.
	namespace AutoEngine
	{
			
		/// Memory pool size definitions for tuning memory usage base on actual use
		struct MemoryPoolSizes
		{
			unsigned int uLowerEnginePoolSize;	///< Memory size for Wwise sound engine rendering components (aka lower engine pool size)
			unsigned int uDefaultPoolSize;		///< Memory size for Wwise sound engine behavioral components (aka default pool size)
			unsigned int uIOMemSize;			///< Memory size for Wwise sound I/O streaming
		};
		static const MemoryPoolSizes DEFAULT_MEM_POOL_SIZES = {16 * 1024 * 1024,2 * 1024 * 1024,2 * 1024 * 1024}; /// Default memory pool size values
		typedef AkGameObjectID		AkAudioObjectID; /// Audio object ID, that is either an emitter or a listener
		static const AkAudioObjectID AK_GLOBAL_AUDIO_OBJECT = 1073741824; /// Default global audio object value

		///////////////////////////////////////////////////////////////////////
		/// @name Initialization and rendering
		//@{
			
		/// Initialize the automotive audio engine.
		/// \warning This function is not thread-safe.
		/// \return 
		/// - AK_Success if the initialization was successful
		/// - AK_SSEInstructionsNotSupported if the machine does not support SSE instruction (only on the PC)
		/// - AK_InsufficientMemory or AK_Fail if there is not enough memory available to initialize the sound engine properly
		/// - AK_InvalidParameter if some parameters are invalid
		/// - AK_Fail if the sound engine is already initialized, or if the provided settings result in insufficient 
		/// resources for the initialization.
		/// \sa
		/// - AK::AutoEngine::Term()
		AKAUTOENGINEDLL_API AKRESULT Init(
			const AkOSChar*   in_pszBasePath,				///< Path for all asset (bank) loading. Mandatory argument.
			const AkOSChar*	in_pszDeviceSharedSet = NULL,	///< Name of device shared set to use for sink plug-ins (NULL uses built-in sink)
			unsigned int in_uSoundEngineBlockSize = 1024,	///< Wwise sound engine block size to use
			unsigned int in_uSampleRate = 48000,			///< Wwise core sample rate to be used (Windows platforms ignores and always uses 48000)
			bool in_bUseRenderThread = true,				///< Controls whether Wwise core will use a dedicated thread for audio rendering (otherwise RenderAudio() just processes the event queue and signals the actual audio rendering thread)
			MemoryPoolSizes in_PoolSizes = DEFAULT_MEM_POOL_SIZES 		///< Memory size for Wwise sound engine memory pools
			);

		/// Terminate the automotive audio engine.
		/// If some sounds are still playing or events are still being processed when this function is 
		///	called, they will be stopped.
		/// \warning This function is not thread-safe.
		/// \warning Before calling Term, you must ensure that no other thread is accessing the audio engine.
		/// \sa 
		/// - AK::AutoEngine::Init()
		AKAUTOENGINEDLL_API void Term();

		/// Process all events in the sound engine's queue.
		/// This method has to be called periodically (usually once per simulation frame).
		/// \sa 
		/// - AK::AutoEngine::PostEvent()
		/// \return Always returns AK_Success
		AKAUTOENGINEDLL_API AKRESULT RenderAudio();
		
		//@}
		
		///////////////////////////////////////////////////////////////////////
		/// @name Audio objects
		//@{

		/// Register an emitter or listener.
		/// \return
		/// - AK_Success if successful
		///	- AK_Fail if the specified AkAudioObjectID is invalid (0 and -1 are invalid)
		/// \remark Registering an audio object twice does nothing. Unregistering it once unregisters it no 
		///			matter how many times it has been registered.
		/// \sa 
		/// - AK::SoundEngine::UnRegisterAudioObject()
		AKAUTOENGINEDLL_API AKRESULT RegisterAudioObject(
			AkAudioObjectID in_audioObjectID,				///< ID of the audio emitter/listener to be registered
			const char * in_pszAudioObjectName = NULL		///< (Optional) Name of the audio emitter/listener (for monitoring purpose)
			);

		/// Unregister an audio emitter or listener.
		/// \return 
		/// - AK_Success if successful
		///	- AK_Fail if the specified AkAudioObjectID is invalid (0 is an invalid ID)
		/// \remark Registering an audio object twice does nothing. Unregistering it once unregisters it no 
		///			matter how many times it has been registered. Unregistering an audio object while it is 
		///			in use is allowed, but the control over the parameters of this audio object is lost.
		///			For example, say a sound associated with this audio object is a 3D moving sound. This sound will 
		///			stop moving when the audio object is unregistered, and there will be no way to regain control over the audio object.
		/// \sa 
		/// - AK::AutoEngine::RegisterAudioObject()
		AKAUTOENGINEDLL_API AKRESULT UnRegisterAudioObject(
			AkAudioObjectID in_audioObjectID				///< ID of the audio object to be unregistered. Use AK_INVALID_AUDIO_OBJECT to unregister all audio emitters.
			);

		/// Set the cartesian coordinates position of an audio object.
		/// \return 
		/// - AK_Success when successful
		/// - AK_InvalidParameter if parameters are not valid.
		AKAUTOENGINEDLL_API AKRESULT SetAudioObjectPosition(
			AkAudioObjectID in_audioObjectID,		///< Audio object identifier
			const AkSoundPosition & in_Position		///< Cartesian coordinates to set relative to car reference point
			);
			
		/// Set an audio emitter's listeners.
		/// Inactive listeners are not computed.
		/// \return Always returns AK_Success
		AKAUTOENGINEDLL_API AKRESULT SetListeners(
			AkAudioObjectID in_audioObjectiD,				///< Audio object identifier
			const AkAudioObjectID* in_pListenerAudioObjs,	///< Array of listener audio object IDs that will be activated for in_audioObjectiD.  Audio objects must have been previously registered.
			AkUInt32 in_uNumListeners						///< Length of array
			);
			
		//@}
		
		///////////////////////////////////////////////////////////////////////
		/// @name Events
		//@{

		/// Post an event to the automotive audio engine by event name.
		/// \return The playing ID of the event launched, or AK_INVALID_PLAYING_ID if posting the event failed
		/// \remarks
		/// \sa 
		/// - AK::AutoEngine::RenderAudio()
		AKAUTOENGINEDLL_API AkPlayingID PostEvent(
			const char* in_pszEventName,										///< Name of the event
			AkAudioObjectID in_audioObjectID = AK_GLOBAL_AUDIO_OBJECT,			///< (Optional) Associated audio object ID
			AkUInt32 in_uFlags = 0,												///< (Optional) Bitmask: see AkCallbackType
			AkCallbackFunc in_pfnCallback = NULL,								///< (Optional) Callback function
			void * in_pCookie = NULL											///< (Optional) Callback cookie that will be sent to the callback function along with additional information
			);
			
		//@}
			
		
		///////////////////////////////////////////////////////////////////////
		/// @name Audio assets
		//@{

		/// Load a bank synchronously.\n
		/// The bank name is passed to the Stream Manager.
		/// A bank load request will be posted, and consumed by the Bank Manager thread.
		/// The function returns when the request has been completely processed.
		/// \return 
		///	- \b AK_Success: Load or unload successful.
		/// - \b AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - \b AK_BankReadError: I/O error.
		/// - \b AK_WrongBankVersion: Invalid bank version. Make sure the version of Wwise used 
		/// to generate the SoundBanks matches that of the SDK you are currently using.
		/// - \b AK_InvalidFile: Specified file could not be opened.
		/// - \b AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - \b AK_Fail: Load or unload failed for a reason other than the preceding ones. (It is most likely a small allocation failure.)
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - All SoundBanks subsequently loaded must come from the same Wwise project as the
		///   initialization bank. If you need to load SoundBanks from a different project, you
		///   must first unload ALL banks, including the initialization bank, then load the
		///   initialization bank from the other project, and finally load banks from that project.
		/// - Codecs and plug-ins must be registered before loading banks that use them.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio emitters, 
		/// posting the event will fail.
		/// - The sound engine internally calls GetIDFromString(in_pszString) to return the correct bank ID.
		/// Therefore, in_pszString should be the real name of the SoundBank (with or without the "bnk" extension - it is trimmed internally),
		/// not the name of the file (if you changed it), nor the full path of the file. 
		/// \sa 
		/// - AK::AutoEngine::UnloadBank()
		AKAUTOENGINEDLL_API AKRESULT LoadBank(
			const char*         in_pszString		    ///< Name of the bank to load
			);

		/// Unload a bank synchronously.\n
		/// \return AK_Success if successful, AK_Fail otherwise. AK_Success is returned when the bank was not loaded.
		/// \remarks
		/// - The sound engine internally calls GetIDFromString(in_pszString) to retrieve the bank ID, 
		/// then it calls the synchronous version of UnloadBank() by ID.
		/// Therefore, in_pszString should be the real name of the SoundBank (with or without the "bnk" extension - it is trimmed internally),
		/// not the name of the file (if you changed it), nor the full path of the file. 
		/// - In order to force the memory deallocation of the bank, sounds that use media from this bank will be stopped. 
		/// This means that streamed sounds or generated sounds will not be stopped.
		/// \sa 
		/// - AK::AutoEngine::LoadBank()
		AKAUTOENGINEDLL_API AKRESULT UnloadBank(
			const char*         in_pszString           ///< Name of the bank to unload
			);
			
		/// Set the current language for localized (language-specific) content name resolution.
		/// Pass a valid null-terminated string, without a trailing slash or backslash. Empty strings are accepted.
		/// \return AK_Success if successful (if language string has less than AK_MAX_LANGUAGE_NAME_SIZE characters). AK_Fail otherwise.
		/// \warning This function is not thread-safe.
		AKAUTOENGINEDLL_API AKRESULT SetCurrentLanguage(
			const AkOSChar* in_pszCurrentLanguage	///< Current language to change to.
			);

		//@}
		
		///////////////////////////////////////////////////////////////////////
		/// @name Runtime context information
		//@{
		
		/// Set the value of a real-time parameter control.
		/// With this function, you may set a runtime parameter value on global scope or on audio object scope. 
		/// Audio object scope supersedes global scope. Runtime parameter values set on global scope are applied to all 
		/// audio emitters that not yet registered, or already registered but not overriden with a value on audio object scope.
		/// To set an audio parameter value on global scope, pass AK_INVALID_AUDIO_OBJECT as the audio object. 
		/// Note that busses ignore RTPCs when they are applied on audio object scope. Thus, you may only change bus 
		/// or bus plugins properties by calling this function with AK_INVALID_AUDIO_OBJECT.
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if in_pszRtpcName is NULL.
		/// \note Strings are case-sensitive. 
		AKAUTOENGINEDLL_API AKRESULT SetRTPCValue(
			const char* in_pszRtpcName,										///< Name of the runtime parameter
			AkRtpcValue in_value, 											///< Value to set
			AkAudioObjectID in_audioObjectiD = AK_GLOBAL_AUDIO_OBJECT		///< (Optional) Associated audio object ID
			);

		/// Set the value of a real-time parameter control (by string name).
		/// With this function, you may set a runtime parameter value on playing id scope. 
		/// Playing id scope supersedes both audio object scope and global scope. 
		/// Note that busses ignore RTPCs when they are applied on playing id scope. Thus, you may only change bus 
		/// or bus plugins properties by calling SetRTPCValue() with AK_INVALID_AUDIO_OBJECT.
		/// \return Always AK_Success
		AKAUTOENGINEDLL_API AKRESULT SetRTPCValueByPlayingID(
			const char* in_pszRtpcName,								///< Name of the runtime parameter
			AkRtpcValue in_value, 									///< Value to set
			AkPlayingID in_playingID								///< Associated playing ID
			);

		/// Reset the value of the runtime parameter to its default value, as specified in the Wwise project.
		/// With this function, you may reset a runtime parameter to its default value on global scope or on audio object scope. 
		/// Audio object scope supersedes global scope. Runtime parameter values reset on global scope are applied to all 
		/// audio emitters that were not overriden with a value on audio object scope.
		/// To reset a runtime parameter value on global scope, pass AK_INVALID_AUDIO_OBJECT as the audio object. 
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if in_pszParamName is NULL.
		/// \note Strings are case-sensitive. 
		/// \sa 
		/// - AK::AutoEngine::SetRTPCValue()
		AKAUTOENGINEDLL_API AKRESULT ResetRTPCValue(
			const char* in_pszRtpcName,										///< Name of the runtime parameter
			AkAudioObjectID in_audioObjectiD = AK_GLOBAL_AUDIO_OBJECT		///< (Optional) Associated audio object ID
			);
			
		/// Stop the current content playing associated to the specified playing ID.
		/// 
		AKAUTOENGINEDLL_API void StopPlayingID(
			AkPlayingID in_playingID					///< Playing ID to be stopped.
			);

		/// Set the state of a switch group.
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if the switch or switch group name was not resolved to an existing ID\n
		/// Make sure that the banks were generated with the "include string" option.
		/// \note Switch requires a properly registered object ID to be provided. If you need a global switch, use SetState() instead and bind your switch container in the authoring to the state instead 
		/// \note Strings are case-sensitive. 
		AKAUTOENGINEDLL_API AKRESULT SetSwitch(
			const char* in_pszSwitchGroup,										///< Name of the switch group
			const char* in_pszSwitchState, 										///< Name of the switch
			AkAudioObjectID in_audioObjectiD									///< Associated audio object ID
			);

		/// Set the state of a state group.
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if the state or state group name was not resolved to an existing ID\n
		/// Make sure that the banks were generated with the "use soundbank names" option.
		/// \note Strings are case-sensitive. 
		AKAUTOENGINEDLL_API AKRESULT SetState(
			const char* in_pszStateGroup,					///< Name of the state group
			const char* in_pszState 						///< Name of the state
			);

		/// Post the specified trigger.
		/// \return 
		/// - AK_Success if successful
		/// Make sure that the banks were generated with the "include string" option.
		/// \note Strings are case-sensitive. 
		AKAUTOENGINEDLL_API AKRESULT PostTrigger(
			const char* in_pszTrigger,			 							///< Name of the trigger
			AkAudioObjectID in_audioObjectID = AK_GLOBAL_AUDIO_OBJECT		///< (Optional) Associated audio object ID
			);
			
		//@}
		
		///////////////////////////////////////////////////////////////////////
		/// @name Audio zones
		//@{

		/// Adds an audio zone consisting of a new output device and an associated listener to the system.  	
		/// \return 
		/// - AK_InvalidParameter: Out of range parameters or unsupported parameter combinations (see parameter list below).
		/// - AK_Success: Parameters are valid.
		AKAUTOENGINEDLL_API AKRESULT AddAudioZone(
			AkUInt32 in_iZoneID,							///< Audio zone identifier (client-defined arbitrarily)
			const AkOSChar*	in_pszDeviceSharedSet,			///< Name of device shared set to use for sink plug-in usage on this output (NULL uses built-in sink). You need to use a plug-in to target a specific device.
			const AkAudioObjectID in_uListenerID,			///< Listener ID to attach to this device (this call will register this audio object ID).  Everything heard by this listener will be sent to this output.  Avoid using listenerID 0, usually reserved for the main output.
			const char * in_pszListenerName = NULL			///< (Optional) Name of the audio listener (for monitoring purpose)
			);

		/// Removes an audio zone from the system.
		/// \return 
		/// - AK_InvalidParameter: Out of range parameters or unsupported parameter combinations (see parameter list below).
		/// - AK_Success: Parameters are valid.
		AKAUTOENGINEDLL_API AKRESULT RemoveAudioZone(
			AkUInt32 in_iZoneID,							///< Audio zone identifier (client-defined arbitrarily)
			const AkAudioObjectID in_uListenerID			///< Listener ID attached to this device (this call will unregister this audio object ID).
			);
			
		//@}
		
		///////////////////////////////////////////////////////////////////////
		/// @name Callbacks
		//@{

		/// Expose audio input plug-in callbacks to client applications.
		/// This allows to feed multiple instances of audio input sources in Wwise audio graph with audio content coming from various audio stream sources (e.g. TTS engine)
		AKAUTOENGINEDLL_API void SetAudioInputStreamCallbacks(
			AkAudioInputPluginExecuteCallbackFunc in_pfnExecCallback,				///< Audio pipeline callback requires audio data feed for a particular playing instance.
			AkAudioInputPluginGetFormatCallbackFunc in_pfnGetFormatCallback = NULL, ///< Optional. Default pipeline format will be used if not used.
			AkAudioInputPluginGetGainCallbackFunc in_pfnGetGainCallback = NULL      ///< Optional. Default pipeline gain will be applied if not used.
			);

		/// Expose plug-in Latency Function to client applications.
		/// This allow for the Client applications to get latency measurement.
		AKAUTOENGINEDLL_API void SetPluginGetLatencyCallbacks(
			AkOutputGetLatencyCallbackFunc in_pfnOutputGetLatencyCallback, 	///< Output Plugins report latency callback
			AkInputGetLatencyCallbackFunc in_pfnInputGetLatencyCallback, 	///< Input Plugins report latency callback
			AkUInt32 in_uLatencyReportThresholdMsec = 5						///< Optional, default to 5 ms. Latency are only report when the latency(abs value) change more than threshold value.
			);
			
		//@}

	}
}



