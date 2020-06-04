package chippermetrics

import "github.com/anki/sai-go-util/metricsutil"

var Metrics = metricsutil.NewRegistry("service.sai_chipper")
var HypothesisMetrics = metricsutil.NewChildRegistry(Metrics, "hypothesis")

// List of metrics for chipper server

var (
	//
	// StreamingIntent (Voice) Requests

	// CountStreamRequest - StreamingIntent (voice) requests
	CountStreamRequest = metricsutil.GetCounter("StreamRequestReceived", Metrics)

	// CountStreamRequestError - requests that did not get an intent-result
	CountStreamRequestError = metricsutil.GetCounter("StreamRequestError", Metrics)

	// CountTextRequest - TextIntent requests
	CountTextRequest = metricsutil.GetCounter("TextRequestReceived", Metrics)

	// CountRecvFirstAudioCanceled et al.
	// errors when receiving the first audio packet from the client.
	// context-canceled, deadline-exceeded errors are fairly common
	CountRecvFirstAudioCanceled   = metricsutil.GetCounter("StreamRecvFirstAudioCanceled", Metrics)
	CountRecvFirstAudioDeadline   = metricsutil.GetCounter("StreamRecvFirstAudioDeadlineExceeded", Metrics)
	CountRecvFirstAudioOtherError = metricsutil.GetCounter("StreamRecvFirstAudioOtherError", Metrics)

	// CountRecvAudioCanceled et al.
	// errors when receiving streaming audio from the client
	// context-canceled errors usually occur after client has the intent-result and closed the connection
	CountRecvAudioCanceled   = metricsutil.GetCounter("StreamRecvAudioCanceled", Metrics)
	CountRecvAudioDeadline   = metricsutil.GetCounter("StreamRecvAudioDeadlineExceeded", Metrics)
	CountRecvAudioOtherError = metricsutil.GetCounter("StreamRecvAudioOtherError", Metrics)

	// CountSendClientUnavailable et al.
	// errors when sending intent-result to client
	// client-unavailable or deadline-exceeded errors are due to client timing out
	CountSendClientUnavailable = metricsutil.GetCounter("StreamSendClientUnavailable", Metrics)
	CountSendClientDeadline    = metricsutil.GetCounter("StreamSendClientDeadlineExceeded", Metrics)
	CountSendClientOtherError  = metricsutil.GetCounter("StreamSendClientOtherError", Metrics)

	//
	// Voice Response

	// CountStreamResponse - successful responses sent to the client
	CountStreamResponse = metricsutil.GetCounter("StreamResponseSent", Metrics)

	// CountStreamResponseError - failure to send result to the client
	CountStreamResponseError = metricsutil.GetCounter("StreamResponseError", Metrics)

	// CountTextResponse - text requests results
	CountTextResponse      = metricsutil.GetCounter("TextResponseSent", Metrics)
	CountTextResponseError = metricsutil.GetCounter("TextResponseError", Metrics)

	//
	// Intent related

	CountMatchFail = metricsutil.GetCounter("IntentMatchFail", Metrics)
	CountNoAudio   = metricsutil.GetCounter("IntentNoAudio", Metrics)
	CountTimeout   = metricsutil.GetCounter("IntentTimeout", Metrics)
	CountMatch     = metricsutil.GetCounter("IntentMatch", Metrics)

	//
	// Dialogflow related

	// CountDF - Dialogflow requests
	CountDF = metricsutil.GetCounter("DialogflowRequestSent", Metrics)

	// CountDFRecvErrors - errors we get from Dialogflow responses
	CountDFRecvErrors = metricsutil.GetCounter("DialogflowRecvError", Metrics)

	// CountDFSendErrors - errors when sending audio to Dialogflow
	CountDFSendErrors = metricsutil.GetCounter("DialogflowSendError", Metrics)

	// CountDFErrors - other errors such as client-creation, permission-denied, audio-decode failures
	CountDFErrors = metricsutil.GetCounter("DialogflowError", Metrics)

	// CountDFNoAudio - intent-result with no text and no intent (we do get is_final)
	CountDFNoAudio = metricsutil.GetCounter("DialogflowIntentNoAudio", Metrics)

	// CountDFTimeoutNoAudio - deadline-exceeded *Without* partial text
	CountDFTimeoutNoAudio = metricsutil.GetCounter("DialogflowIntentTimeoutNoAudio", Metrics)

	// CountDFTimeout - deadline-exceeded *With* partial text
	CountDFTimeout = metricsutil.GetCounter("DialogflowIntentTimeout", Metrics)

	// CountDFMatchFail - intent-matching fail, system_unsupported, system_noaudio, system_timeout
	CountDFMatchFail = metricsutil.GetCounter("DialogflowIntentFail", Metrics)

	// CountDFMatch counts - matched intent-result
	CountDFMatch = metricsutil.GetCounter("DialogflowIntentMatch", Metrics)

	// LatencyDF - intent latency from isFinal/VAD stops to getting result back from DF/Lex
	LatencyDF = metricsutil.GetTimer("DialogflowIntentLatency", Metrics)

	//
	// Microsoft related

	CountMS          = metricsutil.GetCounter("MicrosoftRequestSent", Metrics)
	CountMSErrors    = metricsutil.GetCounter("MicrosoftError", Metrics)
	CountMSMatchFail = metricsutil.GetCounter("MicrosoftIntentFail", Metrics)
	CountMSMatch     = metricsutil.GetCounter("MicrosoftIntentMatch", Metrics)

	//
	// Lex related

	// CountLex - no. of Lex requests
	CountLex = metricsutil.GetCounter("LexRequestSent", Metrics)

	// CountLexPostContentErrors - Lex PostContent SDK errors
	CountLexPostContentErrors = metricsutil.GetCounter("LexPostContentError", Metrics)

	// CountLexErrors - other Lex errors such as nil client, decode audio etc.
	CountLexErrors = metricsutil.GetCounter("LexError", Metrics)

	// CountLexMatchFail - getting nil for intent-result
	CountLexMatchFail = metricsutil.GetCounter("LexIntentFail", Metrics)

	// CountLexMatch - tracks intent-result with non-nil name
	CountLexMatch = metricsutil.GetCounter("LexIntentMatch", Metrics)

	// LatencyLex tracks intent-latency from VAD stops to intent-result returned
	LatencyLex = metricsutil.GetTimer("LexIntentLatency", Metrics)

	// CountLexVadErrors et al. tracks VAD errors
	CountLexVadErrors  = metricsutil.GetCounter("LexVadError", Metrics)
	CountLexVadTimeout = metricsutil.GetCounter("LexVadTimeout", Metrics)

	// TimerWunderground - wunderground latency
	TimerWunderground = metricsutil.GetTimer("WundergroundLatency", Metrics)

	//
	// Chipper-fn

	CountChipperfnSuccess       = metricsutil.GetCounter("ChipperfnSuccess", Metrics)
	CountChipperfnFail          = metricsutil.GetCounter("ChipperfnFail", Metrics)
	CountChipperfnUnknownIntent = metricsutil.GetCounter("ChipperfnUnknownIntent", Metrics)

	//
	// Knowledge Graph related

	// KG requests and request errors
	CountKnowledgeGraphRequest      = metricsutil.GetCounter("KnowledgeGraphReceived", Metrics)
	CountKnowledgeGraphRequestError = metricsutil.GetCounter("KnowledgeGraphError", Metrics)

	// No. of matches and failed matches
	CountKnowledgeGraphMatch     = metricsutil.GetCounter("KnowledgeGraphMatch", Metrics)
	CountKnowledgeGraphMatchFail = metricsutil.GetCounter("KnowledgeGraphFail", Metrics)

	// KG recv_audio and send_audio errors
	CountKGRecvAudioError = metricsutil.GetCounter("KnowledgeGraphRecvAudioError", Metrics)
	CountKGSendAudioError = metricsutil.GetCounter("KnowledgeGraphSendAudioError", Metrics)

	// KG responses - send_client success and errors
	CountKnowledgeGraphResponse      = metricsutil.GetCounter("KnowledgeGraphResponseSent", Metrics)
	CountKnowledgeGraphResponseError = metricsutil.GetCounter("KnowledgeGraphResponseError", Metrics)

	//
	// Houndify related

	CountHoundify             = metricsutil.GetCounter("HoundifyRequestSent", Metrics)
	CountHoundifyErrors       = metricsutil.GetCounter("HoundifyError", Metrics)
	CountHoundifyMatchFail    = metricsutil.GetCounter("HoundifyFail", Metrics)
	CountHoundifyMatch        = metricsutil.GetCounter("HoundifyMatch", Metrics)
	CountHoundifyVadTriggered = metricsutil.GetCounter("HoundifyVadTriggered", Metrics)

	//
	// Connection Check related

	CountConnectionCheck        = metricsutil.GetCounter("ConnectionCheckCount", Metrics)
	CountConnectionCheckError   = metricsutil.GetCounter("ConnectionCheckErrorCount", Metrics)
	CountConnectionCheckSuccess = metricsutil.GetCounter("ConnectionCheckSuccess", Metrics)
	CountConnectionCheckFail    = metricsutil.GetCounter("ConnectionCheckFail", Metrics)
)
