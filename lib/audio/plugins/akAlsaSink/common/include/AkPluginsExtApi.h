
////////////////////////////////////////////////////////////////////////////////////////////
// API external to the plug-in, to be used by the client Application.

/// Callback to get Output Latency
AK_CALLBACK(void, AkOutputGetLatencyCallbackFunc)(
	AkUInt32	in_OutputID,   ///< Playing ID (same that was returned from the PostEvent call).
	bool		in_bIsPrimary, ///< Define main output versus secondary outputs.
	AkUInt64	in_uLatencyUsec///< Latency in usec.
);

AK_CALLBACK(void, AkInputGetLatencyCallbackFunc)(
	AkUInt32	in_PlayingID,   ///< Playing ID (same that was returned from the PostEvent call).
	AkUInt64    in_uLatencyUsec///< Latency in usec.
);


AK_EXTERNAPIFUNC(void, SetAlsaSinkCallbacks)(
		AkOutputGetLatencyCallbackFunc in_pfnGetLatencyCallback,///< Output Latency callback.
		AkUInt32 in_uLatencyReportThresholdMsec///< Optional, default value is 5 ms. Latency is report only when the Latency difference is greater than threashold value.
);

AK_EXTERNAPIFUNC(void, SetPulseAudioSinkCallbacks)(
		AkOutputGetLatencyCallbackFunc in_pfnGetLatencyCallback,///< Output Latency callback.
		AkUInt32 in_uLatencyReportThresholdMsec///< Optional, default value is 5 ms. Latency is report only when the Latency difference is greater than threashold value.
);

AK_EXTERNAPIFUNC(void, SetAlsaSrcCallbacks)(
		AkInputGetLatencyCallbackFunc in_pfnGetLatencyCallback,///< Output Latency callback.
		AkUInt32 in_uLatencyReportThresholdMsec///< Optional, default value is 5 ms. Latency is report only when the Latency difference is greater than threashold value.
);

AK_EXTERNAPIFUNC(void, SetInputStreamSrcCallbacks)(
		AkInputGetLatencyCallbackFunc in_pfnGetLatencyCallback,///< Output Latency callback.
		AkUInt32 in_uLatencyReportThresholdMsec///< Optional, default value is 5 ms. Latency is report only when the Latency difference is greater than threashold value.
);

////////////////////////////////////////////////////////////////////////////////////////////

