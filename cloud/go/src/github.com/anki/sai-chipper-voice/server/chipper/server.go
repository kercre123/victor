package chipper

import (
	"context"
	"encoding/json"
	"errors"
	"strings"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/anki/sai-go-util/ctxutil"
	"github.com/anki/sai-go-util/log"

	"github.com/anki/sai-chipper-voice/conversation/dialogflow"
	"github.com/anki/sai-chipper-voice/hstore"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/requestrouter"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	FallbackIntent  = "intent_system_unsupported"
	NoAudioIntent   = "intent_system_noaudio" // when there's no detected audio
	TimeoutIntent   = "intent_system_timeout" // when there's asr result, but intent-services timeout
	KGIntent        = "intent_knowledge_graph"
	KGMode          = "KG"
	MaxAudioSamples = 200 // 20 secs of 100ms chunks
)

var LanguageCodes = map[pb.LanguageCode]string{
	pb.LanguageCode_ENGLISH_US: "en-US",
	pb.LanguageCode_ENGLISH_UK: "en-GB",
	pb.LanguageCode_ENGLISH_AU: "en-AU",
	//pb.LanguageCode_GERMAN:     "de-DE", // de and fr delayed till Spring 2019
	//pb.LanguageCode_FRENCH:     "fr-FR",
}

var AudioCodecs = map[pb.AudioEncoding]dialogflow.AudioEncoding{
	pb.AudioEncoding_OGG_OPUS:   dialogflow.OggEncoding,
	pb.AudioEncoding_LINEAR_PCM: dialogflow.LinearEncoding,
}

//
// Server implements gRPC ChipperServer interface
//

func NewServer(options ...ServerOpts) *Server {
	server := &Server{
		logTranscript: false,
		saveAudio:     false,
		intentSvcEnabled: map[pb.IntentService]bool{
			pb.IntentService_DIALOGFLOW: false,
			pb.IntentService_BING_LUIS:  false,
			pb.IntentService_LEX:        false,
		},
	}

	for _, opt := range options {
		opt(server)
	}
	return server
}

func (s *Server) withServerOpts(options ...ServerOpts) {
	for _, opt := range options {
		opt(s)
	}
}

//
// StreamingIntent API
//

func (s *Server) StreamingIntent(stream pb.ChipperGrpc_StreamingIntentServer) error {

	// received request time
	recvTime := time.Now()
	chippermetrics.CountStreamRequest.Inc(1)

	// get the first request so we know which intent-service to use
	req, err := stream.Recv()
	if err != nil {
		errCode := grpc.Code(err)
		alog.Warn{
			"action":     "recv_first_request",
			"status":     alog.StatusError,
			"req_type":   "audio",
			"error":      err,
			"error_code": errCode,
		}.LogC(stream.Context())
		if errCode == codes.Canceled {
			chippermetrics.CountRecvFirstAudioCanceled.Inc(1)
		} else if errCode == codes.DeadlineExceeded {
			chippermetrics.CountRecvFirstAudioDeadline.Inc(1)
		} else {
			chippermetrics.CountRecvFirstAudioOtherError.Inc(1)
		}
		chippermetrics.CountStreamRequestError.Inc(1)
		return err
	}

	// get device-id from JWT token or protobuf
	deviceId, isEsn, err := getDeviceId(stream.Context(), req.DeviceId)
	if err != nil {
		alog.Warn{"action": "get_device_id", "status": alog.StatusError, "error": err}.LogC(stream.Context())
		chippermetrics.CountStreamRequestError.Inc(1)
		return err
	}

	// extract normalized firmware string for request routing
	fwVersion, dasRobotVersion := requestrouter.ExtractVersionBuildString(req.FirmwareVersion)
	alog.Debug{
		"action":            "extract_fw_string",
		"fw_version":        req.FirmwareVersion,
		"extracted_version": fwVersion,
		"das_version":       dasRobotVersion,
	}.Log()

	alog.Info{
		"action":          "recv_first_request",
		"req_type":        "audio",
		"frame":           "1",
		"device":          deviceId,
		"session":         req.Session,
		"service":         req.IntentService,
		"encoding":        req.AudioEncoding,
		"lang":            req.LanguageCode,
		"mode":            req.Mode,
		"fw_version":      req.FirmwareVersion,
		"fw_version_norm": fwVersion,
		"bytes_recv":      len(req.InputAudio),
		"boot_id":         strings.TrimSpace(req.BootId),
		"time_elapsed_ms": durationMs(recvTime),
	}.LogC(stream.Context())

	// validate parameters
	langCode, err := validateLanguage(req.LanguageCode)
	if err != nil {
		chippermetrics.CountStreamRequestError.Inc(1)
		return err
	}

	audioCodec, err := validateAudioEncoding(req.AudioEncoding)
	if err != nil {
		chippermetrics.CountStreamRequestError.Inc(1)
		return err
	}

	// get intent-version to use based on lang, mode, fw and robotid
	routeInput := requestrouter.RequestRouteInput{
		Requested: req.IntentService,
		LangCode:  req.LanguageCode,
		Mode:      req.Mode,
		RobotId:   deviceId,
		FwVersion: fwVersion,
	}
	alog.Debug{"route": &routeInput}.Log()

	routedVersion, err := s.requestRouter.Route(stream.Context(), routeInput)
	if err != nil {
		alog.Error{
			"action":      "route_service",
			"status":      alog.StatusError,
			"err":         err,
			"route_input": &routeInput,
			"msg":         "no intent version found, call Ben or KingShy or Adam or Tom (any)!!",
		}.LogC(stream.Context())
		chippermetrics.CountStreamRequestError.Inc(1)
		return status.Errorf(codes.NotFound, "no intent version found")

	}

	alog.Debug{
		"action":         "route_service",
		"device":         deviceId,
		"session":        req.Session,
		"intent_service": routedVersion.IntentVersion.IntentSvc,
	}.LogC(stream.Context())

	intentReq := chipperRequest{
		stream:          stream,
		firstReq:        req,
		device:          deviceId,
		deviceIsEsn:     isEsn,
		session:         req.Session,
		langString:      langCode,
		audioCodec:      audioCodec,
		reqTime:         recvTime,
		asr:             req.SpeechOnly,
		mode:            req.Mode,
		langCode:        req.LanguageCode,
		ctx:             stream.Context(),
		version:         routedVersion,
		dasRobotVersion: dasRobotVersion,
	}

	var result *chipperResult = nil
	var svc string

	alog.Info{"action": "process_request", "svc": routedVersion.IntentVersion.IntentSvc, "status": alog.StatusStart}.LogC(intentReq.ctx)

	switch routedVersion.IntentVersion.IntentSvc {
	case pb.IntentService_DIALOGFLOW:
		result = s.processRequestDialogflow(intentReq)
		svc = GOOG
	case pb.IntentService_BING_LUIS:
		result = s.processRequestMicrosoft(intentReq)
		svc = MS
	case pb.IntentService_LEX:
		result = s.processRequestLex(intentReq)
		svc = LEX
	default:
		return errors.New("Invalid intent service")
	}

	if result.err != nil {
		alog.Info{"action": "process_request", "svc": routedVersion.IntentVersion.IntentSvc, "status": alog.StatusError, "error": result.err}.LogC(intentReq.ctx)
		chippermetrics.CountStreamRequestError.Inc(1)
	}

	if result.intent != nil {
		// save audio for dev
		if s.saveAudio && intentReq.firstReq.SaveAudio && result.audio != nil {
			requestId := ctxutil.GetRequestId(stream.Context())
			s.audioStoreClient.Save(stream.Context(), result.audio, result.intent, audioCodec.String(), svc, requestId, recvTime)
		}

		// save DAS event
		if !req.SkipDas && s.dasClient != nil {
			e := s.dasClient.CreateEvent(result, intentReq, RequestTypeAudio, req.BootId)
			s.dasClient.Send(stream.Context(), e)
		}

		// write to Hypothesis Store
		if s.hypostoreClient != nil {
			paramString, _ := json.Marshal(result.intent.IntentResult.Parameters)
			err := s.hypostoreClient.Put(
				hstore.Record{
					Hypothesis:       result.intent.IntentResult.QueryText,
					Service:          svc,
					Confidence:       result.intent.IntentResult.SpeechConfidence,
					IntentMatch:      result.intent.IntentResult.Action,
					IntentConfidence: result.intent.IntentResult.IntentConfidence,
					ParsedEntity:     string(paramString),
					RawEntity:        result.rawParams,
					LangCode:         req.LanguageCode.String(),
					Mode:             req.Mode.String(),
					RequestTime:      hypothesisStoreTimestamp(recvTime),
				},
			)
			if err != nil {
				alog.Warn{
					"action":  "hypothesis_put",
					"status":  alog.StatusError,
					"error":   err,
					"device":  deviceId,
					"session": req.Session,
				}.LogC(stream.Context())
			}
		}
	}

	alog.Debug{
		"action":    "streaming_intent_done",
		"has_error": result.err != nil,
	}.LogC(stream.Context())

	return result.err
}

//
// TextIntent API
//

func (s *Server) TextIntent(ctx context.Context, req *pb.TextRequest) (*pb.IntentResponse, error) {

	reqTime := time.Now()
	chippermetrics.CountTextRequest.Inc(1)

	// get device-id from JWT token or protobuf
	deviceId, isEsn, err := getDeviceId(ctx, req.DeviceId)
	if err != nil {
		alog.Warn{"action": "get_device_id", "status": alog.StatusError, "error": err}.LogC(ctx)
		return nil, err
	}

	id := createSessionHash(deviceId, req.Session)

	langCode, err := validateLanguage(req.LanguageCode)
	if err != nil {
		return nil, err
	}

	// get intent-version to use based on lang, mode, fw and robotid
	fwVersion, dasRobotVersion := requestrouter.ExtractVersionBuildString(req.FirmwareVersion)
	alog.Info{
		"action":            "extract_fw_string",
		"fw_version":        req.FirmwareVersion,
		"extracted_version": fwVersion,
	}.LogC(ctx)

	routedVersion, err := s.requestRouter.Route(ctx, requestrouter.RequestRouteInput{
		Requested: req.IntentService,
		LangCode:  req.LanguageCode,
		Mode:      req.Mode,
		RobotId:   deviceId,
		FwVersion: fwVersion,
	})

	alog.Debug{"routed": routedVersion}.LogC(ctx)

	if err != nil {
		alog.Error{
			"action": "text_intent",
			"status": alog.StatusError,
			"err":    err,
			"msg":    "no intent version found, call Ben or KingShy!!",
		}.LogC(ctx)
		return nil, status.Errorf(codes.NotFound, "no intent version found")
	}

	intentReq := chipperRequest{
		device:          deviceId,
		deviceIsEsn:     isEsn,
		session:         req.Session,
		langString:      langCode,
		reqTime:         reqTime,
		audioCodec:      dialogflow.UnSpecifiedEncoding,
		mode:            req.Mode,
		ctx:             ctx,
		version:         routedVersion,
		dasRobotVersion: dasRobotVersion,
	}

	var result *chipperResult
	var svc string

	switch routedVersion.IntentVersion.IntentSvc {
	case pb.IntentService_DIALOGFLOW:
		svc = GOOG
		// projectName, credEnv := s.ChooseDialogflowProject(intentReq.requestId, req.LanguageCode, req.Mode)
		v := routedVersion.IntentVersion
		dfClient, err := dialogflow.NewSessionClient(
			ctx,
			v.DFProjectName,
			v.DFVersion,
			id,
			v.DFCredsKey)

		if err != nil {
			alog.Warn{
				"action": "create_dialogflow_client",
				"status": alog.StatusFail,
				"error":  err,
			}.LogC(ctx)
			result = &chipperResult{intent: nil, err: err}
		} else {
			result = s.DialogflowTextIntent(dfClient, req.TextInput, intentReq)
		}
	case pb.IntentService_BING_LUIS:
		svc = MS
		intent, rErr := s.luisClient.GetIntent(req.TextInput)
		if rErr == nil {
			res, paramsString := s.getLuisResult(intentReq, intent)
			alog.Debug{"luis_param": res.IntentResult.Parameters}.LogC(ctx)

			result = &chipperResult{intent: res, err: rErr, rawParams: paramsString}

		} else {
			result = &chipperResult{intent: nil, err: rErr}
		}

	case pb.IntentService_LEX:
		svc = LEX
		result = s.PerformLexTextRequest(req.TextInput, intentReq)

	default:
		svc = "error"
		result = &chipperResult{
			intent: nil,
			err:    status.Errorf(codes.InvalidArgument, "invalid intent service"),
		}
	}

	alog.Debug{"error": result.err}.LogC(ctx)

	if result.err != nil {
		alog.Warn{
			"action": "text_request",
			"status": alog.StatusFail,
			"svc":    svc,
			"error":  result.err,
		}.LogC(ctx)
		chippermetrics.CountTextResponseError.Inc(1)
		return nil, result.err
	}

	if !req.SkipDas && result.intent != nil && s.dasClient != nil {
		bootId := ""
		e := s.dasClient.CreateEvent(result, intentReq, RequestTypeText, bootId) // save DAS event
		s.dasClient.Send(ctx, e)
	}

	if s.hypostoreClient != nil {
		// write to Hypothesis Store
		paramString, _ := json.Marshal(result.intent.IntentResult.Parameters)
		err := s.hypostoreClient.Put(
			hstore.Record{
				Hypothesis:       result.intent.IntentResult.QueryText,
				Service:          svc,
				Confidence:       result.intent.IntentResult.SpeechConfidence,
				IntentMatch:      result.intent.IntentResult.Action,
				IntentConfidence: result.intent.IntentResult.IntentConfidence,
				ParsedEntity:     string(paramString),
				RawEntity:        result.rawParams,
				LangCode:         req.LanguageCode.String(),
				Mode:             req.Mode.String(),
				RequestTime:      hypothesisStoreTimestamp(reqTime),
			},
		)
		if err != nil {
			alog.Warn{
				"action":  "hypothesis_put",
				"status":  alog.StatusError,
				"error":   err,
				"device":  deviceId,
				"session": req.Session,
			}.LogC(ctx)
		}
	}
	chippermetrics.CountTextResponse.Inc(1)
	return result.intent, nil
}

//
// Knowledge Graph API
//

func (s *Server) StreamingKnowledgeGraph(stream pb.ChipperGrpc_StreamingKnowledgeGraphServer) error {

	recvTime := time.Now()
	chippermetrics.CountKnowledgeGraphRequest.Inc(1)

	// get the first request so we know which intent-service to use
	req, err := stream.Recv()
	if err != nil {
		alog.Warn{"action": "receive_first_request_knowledge_graph", "status": "unexpected_error", "error": err}.LogC(stream.Context())
		chippermetrics.CountKnowledgeGraphRequestError.Inc(1)
		return err
	}

	// get device-id from JWT token or protobuf
	deviceId, isEsn, err := getDeviceId(stream.Context(), req.DeviceId)
	if err != nil {
		alog.Warn{"action": "get_device_id", "status": alog.StatusError, "error": err}.LogC(stream.Context())
		chippermetrics.CountKnowledgeGraphRequestError.Inc(1)
		return err
	}

	alog.Info{
		"action":          "recv_first_request",
		"status":          alog.StatusOK,
		"svc":             HOUNDIFY,
		"frame":           "1",
		"time_elapsed_ms": durationMs(recvTime),
		"device":          deviceId,
		"session":         req.Session,
		"encoding":        req.AudioEncoding,
		"lang":            req.LanguageCode,
		"num_bytes":       len(req.InputAudio),
		"boot_id":         strings.TrimSpace(req.BootId),
		"timezone":        strings.TrimSpace(req.Timezone),
	}.LogC(stream.Context())

	// validate parameters
	langCode, err := validateLanguage(req.LanguageCode)
	if err != nil {
		chippermetrics.CountKnowledgeGraphRequestError.Inc(1)
		return err
	}

	audioCodec, err := validateAudioEncoding(req.AudioEncoding)
	if err != nil {
		chippermetrics.CountKnowledgeGraphRequestError.Inc(1)
		return err
	}

	fwVersion, dasRobotVersion := requestrouter.ExtractVersionBuildString(req.FirmwareVersion)

	intentReq := kgRequest{
		stream:          stream,
		firstReq:        req,
		device:          deviceId,
		deviceIsEsn:     isEsn,
		session:         req.Session,
		langString:      langCode,
		audioCodec:      audioCodec,
		reqTime:         recvTime,
		fw:              fwVersion,
		dasRobotVersion: dasRobotVersion,
		timezone:        strings.TrimSpace(req.Timezone),
	}

	alog.Info{"action": "process_request", "svc": HOUNDIFY, "status": alog.StatusStart}.LogC(stream.Context())

	result := s.processRequestHoundify(intentReq)

	if result.err != nil {
		alog.Info{"action": "process_request", "svc": HOUNDIFY, "status": alog.StatusError, "error": result.err}.LogC(stream.Context())
		chippermetrics.CountKnowledgeGraphRequestError.Inc(1)
	}

	if result.kg != nil {
		// save audio for dev
		if s.saveAudio && intentReq.firstReq.SaveAudio && result.audio != nil {
			requestId := ctxutil.GetRequestId(stream.Context())
			s.audioStoreClient.SaveKG(stream.Context(), result.audio, result.kg, audioCodec.String(), HOUNDIFY, requestId, recvTime)
		}

		// save DAS event
		if !req.SkipDas && result.kg != nil && s.dasClient != nil {
			e := s.dasClient.CreateKnowledgeGraphEvent(result, intentReq, req.BootId) // save DAS event
			s.dasClient.Send(stream.Context(), e)
		}

		if s.hypostoreClient != nil {

			// write to Hypothesis Store
			kgJson, err := json.Marshal(result.kgResponse)
			kgRaw := ""
			if err != nil {
				alog.Warn{
					"action": "marshal_kg_response",
					"status": alog.StatusFail,
					"svc":    HOUNDIFY,
					"error":  err,
				}.LogC(stream.Context())
			} else {
				kgRaw = string(kgJson)
			}
			record := hstore.Record{
				Hypothesis:    result.kg.QueryText,
				Service:       HOUNDIFY,
				Confidence:    float32(result.kgResponse.Disambiguation.ChoiceData[0].ConfidenceScore),
				IntentMatch:   KGIntent,
				LangCode:      req.LanguageCode.String(),
				Mode:          KGMode,
				RequestTime:   hypothesisStoreTimestamp(recvTime),
				KgResponse:    result.kg.SpokenText,
				RawKgResponse: kgRaw,
			}

			if result.kgResponse.AllResults[0].UnderstandingConfidence != nil {
				record.IntentConfidence = float32(*result.kgResponse.AllResults[0].UnderstandingConfidence)
			}

			err = s.hypostoreClient.Put(record)
			if err != nil {
				alog.Warn{
					"action":  "hypothesis_put",
					"status":  alog.StatusError,
					"svc":     HOUNDIFY,
					"error":   err,
					"device":  deviceId,
					"session": req.Session,
				}.LogC(stream.Context())
			}
		}
	}
	return result.err
}

func (in *Server) StreamingConnectionCheck(stream pb.ChipperGrpc_StreamingConnectionCheckServer) error {
	chippermetrics.CountConnectionCheck.Inc(1)

	// get the first request so we know which intent-service to use
	req, err := stream.Recv()
	if err != nil {
		alog.Warn{
			"action": "recv_first_request",
			"status": "unexpected_error",
			"svc":    CHECK,
			"error":  err,
			"msg":    "connection first request fail",
		}.LogC(stream.Context())
		chippermetrics.CountConnectionCheckError.Inc(1)
		return err
	}

	// get device-id from JWT token or protobuf
	deviceId, isEsn, err := getDeviceId(stream.Context(), req.DeviceId)
	if err != nil {
		alog.Warn{"action": "get_device_id", "status": alog.StatusError, "error": err}.LogC(stream.Context())
		chippermetrics.CountConnectionCheckError.Inc(1)
		return err
	}

	recvTime := time.Now()
	requestId := ctxutil.GetRequestId(stream.Context())
	alog.Info{
		"action":          "recv_first_request",
		"status":          alog.StatusOK,
		"svc":             CHECK,
		"frame":           "1",
		"time_elapsed_ms": durationMs(recvTime),
		"device":          deviceId,
		"session":         req.Session,
		"bytes_recv":      len(req.InputAudio),
	}.LogC(stream.Context())

	connectionReq := connectionRequest{
		stream:          stream,
		firstReq:        req,
		device:          deviceId,
		session:         req.Session,
		deviceIsEsn:     isEsn,
		reqTime:         recvTime,
		requestId:       requestId,
		totalAudioMs:    req.TotalAudioMs,
		audioPerRequest: req.AudioPerRequest,
	}

	return in.processRequestConnectionCheck(connectionReq)
}
