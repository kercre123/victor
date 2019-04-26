package chipper

import (
	"bytes"
	"context"
	"encoding/json"
	"io/ioutil"
	"os"
	"time"

	google_protobuf4 "github.com/golang/protobuf/ptypes/struct"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/anki/opus-go/opus"
	"github.com/anki/sai-go-util/log"

	"github.com/anki/sai-chipper-voice/audiostore"
	df "github.com/anki/sai-chipper-voice/conversation/dialogflow"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	GOOG                = "dialogflow"
	streamClientTimeout = 8 * time.Second
)

type DialogflowConfig struct {
	ProjectName *string
	Creds       *string
	CredEnvVar  string
}

type dialogflowCred struct {
	CredType    string `json:"type,omitempty"`
	ProjId      string `json:"project_id,omitempty"`
	ClientEmail string `json:"client_email,omitempty"`
}

func InitDialogflow(configs ...DialogflowConfig) error {
	for _, config := range configs {
		if *config.Creds != "" {
			f, err := ioutil.TempFile("", config.CredEnvVar)
			if err != nil {
				return err
			}
			credFilename := f.Name()
			f.Write([]byte(*config.Creds))
			f.Close()
			os.Setenv(config.CredEnvVar, credFilename)
		}
	}
	return nil
}

func (s *Server) newDialogflowStreamClient(ctx context.Context, req chipperRequest) (df.StreamingClient, error) {

	// choose voice-command or game project
	// projName, credEnv := s.ChooseDialogflowProject(req.requestId, req.langCode, req.mode)

	// setup client configuration
	session := createSessionHash(req.device, req.session)
	config := df.IntentConfig{
		SingleUtterance: req.firstReq.SingleUtterance,
		Language:        req.langString,
		// AudioEncoding is ALWAYS set to LinearEncoding.
		// If we get opus audio, it will be decompressed before being sent to DF
		AudioEncoding: df.LinearEncoding,
	}

	// create Dialogflow streaming client
	v := req.version.IntentVersion
	client, err := df.NewStreamingClient(ctx, v.DFProjectName, v.DFVersion, session, v.DFCredsKey, config)

	if err != nil {
		alog.Warn{
			"action":  "create_dialogflow_stream_client",
			"status":  alog.StatusFail,
			"error":   err,
			"svc":     GOOG,
			"device":  req.device,
			"session": req.session,
		}.LogC(ctx)
		chippermetrics.CountDFErrors.Inc(1)
		return nil, err
	}

	return client, nil
}

// entry
// Dialogflow consists of two parts: ASR + Intent Classification
func (s *Server) processRequestDialogflow(req chipperRequest) *chipperResult {
	chippermetrics.CountDF.Inc(1)

	// create context with timeout
	ctx, cancel := context.WithTimeout(req.ctx, streamClientTimeout)
	defer cancel()

	dfClient, err := s.newDialogflowStreamClient(ctx, req)
	if err != nil {
		alog.Error{"action": "create_dialogflow_client", "status": alog.StatusError, "error": err}.LogC(req.ctx)
		return &chipperResult{
			err: status.Error(codes.Unavailable, "fail to connect to Dialogflow, "+err.Error()),
		}
	}
	defer dfClient.Close()

	result := s.dialogflowStreamingIntent(dfClient, req)

	// for results with proper intents, we call Dialogflow one more time to get the EOF error
	// so we can get an access log entry for metrics
	// Note: we don't do this for timeout intent, the DeadlineExceeded error already generated the log
	if result.err == nil && result.intent.IntentResult.Action != TimeoutIntent {
		dfClient.Recv()
	}

	return result
}

// main processing loop
func (s *Server) dialogflowStreamingIntent(client df.StreamingClient, req chipperRequest) *chipperResult {

	firstSend := req.firstReq.GetInputAudio()
	var oggDecoderStream *opus.OggStream
	// setup decompression if required
	if req.audioCodec == df.OggEncoding {
		oggDecoderStream = &opus.OggStream{}
		decoded, err := oggDecoderStream.Decode(firstSend)
		if err != nil {
			alog.Warn{
				"action":  "decode_audio",
				"status":  alog.StatusFail,
				"svc":     GOOG,
				"error":   err,
				"device":  req.device,
				"session": req.session,
				"msg":     "fail to decode opus audio for dialogflow, exiting",
			}.LogC(req.ctx)
			return &chipperResult{
				err: status.Errorf(codes.Internal, "fail to decode audio for dialogflow"),
			}
		}
		firstSend = decoded
	}

	// send first audio bytes
	err := client.Send(firstSend)
	if err != nil {
		alog.Warn{
			"action":  "send_first_audio",
			"status":  alog.StatusFail,
			"svc":     GOOG,
			"error":   err,
			"device":  req.device,
			"session": req.session,
		}.LogC(req.ctx)
		chippermetrics.CountDFSendErrors.Inc(1)
		return &chipperResult{err: err}
	}

	sentBytes := len(firstSend)

	alog.Debug{
		"action":          "send_first_audio",
		"status":          alog.StatusOK,
		"svc":             GOOG,
		"time_elapsed_ms": durationMs(req.reqTime),
		"device":          req.device,
		"session":         req.session,
	}.LogC(req.ctx)

	// done channel indicates that we got is_final result, stop sending audio
	done := make(chan struct{}, 1)

	// waitc for sendAudioDialogflow to exit before returning
	waitc := make(chan struct{})

	// channel to pass audio bytes we want to save (dev only)
	saveAudioBytes := make(chan []byte, MaxAudioSamples)

	// audioLength channel to get request audio duration
	audioLength := make(chan time.Duration, 1)

	//
	// send Audio
	//

	go sendAudioDialogflow(
		client,
		req,
		done, waitc,
		s.saveAudio, saveAudioBytes, audioLength,
		oggDecoderStream,
		sentBytes,
	)

	// dev only - save first audio packet
	var audioBytes bytes.Buffer
	if s.saveAudio {
		audioBytes = *bytes.NewBuffer(firstSend)
	}

	//
	// process dialogflow responses
	//

	var res *pb.IntentResponse = nil
	paramsString := ""
	speechText := ""
	speechConfidence := float32(0)

	var isFinalTime *time.Time

dialogflowLoop:
	for {
		select {
		case audio := <-saveAudioBytes:
			if s.saveAudio {
				audioBytes.Write(audio)
			}
		default:

			//
			// receive dialogflow response (blocking), the sequence of responses:
			// 1. a series of partial speech transcript with isFinal=false
			// 2. Full speech transcript with isFinal=true (end-of-speech detected)
			// 3. finally, intent-result, with isFinal=true
			//

			isFinal, result, err := client.Recv()

			if err != nil {

				client.CloseSend()

				// other errors
				if grpc.Code(err) != codes.DeadlineExceeded {
					logDialogflowResponseError(err, s.dialogflowProject, req, df.QueryTypeStreaming)
					return &chipperResult{err: err}
				}

				// process timeout
				res, paramsString = s.getDialogflowTimeoutResult(req, speechText, speechConfidence)

				// send timeout response to client
				sendErr := req.stream.Send(res)
				if sendErr != nil {
					logSendClientError(req, GOOG, SendClientTimeoutErrorMsg, sendErr, isFinalTime, float32(0))
				} else {
					chippermetrics.CountStreamResponse.Inc(1)
				}

				if s.saveAudio && len(saveAudioBytes) > 0 {
					remaining := flushAudio(saveAudioBytes, req, GOOG)
					audioBytes.Write(remaining.Bytes())
				}
				audioDuration := <-audioLength

				return &chipperResult{
					intent:      res,
					audio:       &audioBytes,
					err:         sendErr,
					rawParams:   paramsString,
					audioLength: &audioDuration,
				}
			}

			if result == nil {
				continue
			}

			respType := result.ResponseType

			// Get transcript and confidence for speech result
			if respType == df.ResponseTypePartialSpeech || respType == df.ResponseTypeFinalSpeech {
				speechText = result.QueryText
				speechConfidence = result.SpeechConfidence
			}

			// Check speech recognition result
			// if response-type is ResponseTypeFinalSpeech, it means the ASR part is done.
			//
			// With SingleUtterance = true, the next response will contain the intent-result,
			// and Dialogflow will close the stream.
			//
			// With SingleUtterance = false (current default for robot), we need to close the Dialogflow
			// stream to indicate that no more audio is being streamed. Dialogflow will then  proceed with
			// the intent-classification. The next response then contains the intent-result.
			// we send a "done" signal to sendAudioDialogflow(), which will stop the audio stream.
			if isFinal && respType == df.ResponseTypeFinalSpeech {
				finalTime := time.Now()
				isFinalTime = &finalTime

				logEntry := alog.Info{
					"action":      "recv_dialogflow",
					"status":      "is_final",
					"svc":         GOOG,
					"device":      req.device,
					"session":     req.session,
					"speech_conf": speechConfidence,
					"msg":         "recv is_final for speech, send done signal to send-audio routine",
				}
				if s.logTranscript {
					logEntry["speech_text"] = speechText
				}
				logEntry.LogC(req.ctx)
				done <- struct{}{}

				// loop one more time to get final intent result
				continue
			}

			// Parse final intent result
			if isFinal && respType == df.ResponseTypeIntent {
				res, paramsString = s.getDialogflowResult(req, result, speechText, speechConfidence)
				res.IntentResult.SpeechConfidence = speechConfidence

				// Send result back to chipper's client
				err = req.stream.Send(res)

				if err != nil {
					logSendClientError(req, GOOG, SendClientErrorMsg, err, isFinalTime, float32(0))

					if s.saveAudio && len(saveAudioBytes) > 0 {
						remaining := flushAudio(saveAudioBytes, req, GOOG)
						audioBytes.Write(remaining.Bytes())
					}
					audioDuration := <-audioLength
					return &chipperResult{
						intent:      res,
						audio:       &audioBytes,
						err:         err,
						rawParams:   paramsString,
						audioLength: &audioDuration,
					}
				}

				// last result, exit loop
				if isFinal {
					chippermetrics.CountStreamResponse.Inc(1)
					logEntry := alog.Info{
						"action":  "send_client",
						"status":  alog.StatusOK,
						"svc":     GOOG,
						"device":  req.device,
						"session": req.session,
						"intent":  res.IntentResult.Action,
						"msg":     "sent dialogflow result to client",
					}
					if isFinalTime != nil {
						logEntry["intent_latency_ms"] = durationMs(*isFinalTime)
						chippermetrics.LatencyDF.UpdateSince(*isFinalTime)
					}
					if s.logTranscript {
						logEntry["query"] = res.IntentResult.QueryText
					}
					logEntry.LogC(req.ctx)
					break dialogflowLoop
				}
			}
		}
	}

	<-waitc

	if s.saveAudio && len(saveAudioBytes) > 0 {
		remaining := flushAudio(saveAudioBytes, req, GOOG)
		audioBytes.Write(remaining.Bytes())
	}

	audioDuration := <-audioLength
	return &chipperResult{
		intent:      res,
		audio:       &audioBytes,
		err:         nil,
		rawParams:   paramsString,
		audioLength: &audioDuration,
	}
}

func sendAudioDialogflow(
	dialogflowClient df.StreamingClient,
	reqInfo chipperRequest,
	done, waitc chan struct{},
	saveAudio bool,
	audioBytes chan []byte,
	audioLength chan time.Duration,
	oggDecoderStream *opus.OggStream,
	sentBytes int) {

	defer close(waitc)

	stopSending := false
	frame := 1
	totalBytes := len(reqInfo.firstReq.InputAudio) // bytes recv
	elapsed := audioElapsedMs{float32(0), float32(0), float32(0), float32(0)}

	for {
		select {

		case <-done:
			// an isFinal speech result is received in the main loop
			alog.Info{
				"action":           "recv_done_signal",
				"status":           "stop_sending",
				"svc":              GOOG,
				"device":           reqInfo.device,
				"session":          reqInfo.session,
				"send_duration_ms": durationMs(reqInfo.reqTime),
				"msg":              "receive is_final, stop sending audio and close stream",
			}.LogC(reqInfo.ctx)

			stopSending = true
			dialogflowClient.CloseSend()

		default:

			//
			// receive audio from client
			//

			req, err := reqInfo.stream.Recv()

			if err != nil {
				dialogflowClient.CloseSend()
				audioLength <- time.Since(reqInfo.reqTime)
				logRecvAudioErrors(reqInfo, GOOG, err)
				logRecvAudioSummary(reqInfo, GOOG, frame, totalBytes, sentBytes, elapsed)
				return
			}

			frame++
			totalBytes += len(req.InputAudio)
			elapsed.total = durationMs(reqInfo.reqTime)
			if frame == 10 {
				elapsed.first10 = elapsed.total
			} else if frame == 20 {
				elapsed.first20 = elapsed.total
			}

			if !stopSending {
				//
				// Send Audio to Dialogflow
				//
				// decompress first if needed
				audioToSend := req.GetInputAudio()
				if oggDecoderStream != nil {
					decoded, err := oggDecoderStream.Decode(audioToSend)
					if err != nil {
						alog.Warn{"action": "decode_audio",
							"status":  alog.StatusFail,
							"svc":     GOOG,
							"error":   err,
							"device":  reqInfo.device,
							"session": reqInfo.session,
							"msg":     "fail to uncompress audio for dialogflow, exiting",
						}.LogC(reqInfo.ctx)
						chippermetrics.CountDFErrors.Inc(1)
						audioLength <- time.Since(reqInfo.reqTime)
						logRecvAudioSummary(reqInfo, GOOG, frame, totalBytes, sentBytes, elapsed)

						return
					}
					audioToSend = decoded
				}

				err := dialogflowClient.Send(audioToSend)
				sentBytes += len(audioToSend)
				elapsed.sent = elapsed.total

				if err != nil {
					alog.Warn{"action": "send_audio",
						"svc":              GOOG,
						"status":           alog.StatusError,
						"error":            err,
						"device":           reqInfo.device,
						"session":          reqInfo.session,
						"msg":              "fail to send audio to dialogflow, exiting",
						"send_duration_ms": durationMs(reqInfo.reqTime),
					}.LogC(reqInfo.ctx)
					chippermetrics.CountDFSendErrors.Inc(1)
					audioLength <- time.Since(reqInfo.reqTime)
					logRecvAudioSummary(reqInfo, GOOG, frame, totalBytes, sentBytes, elapsed)

					return
				}

				if saveAudio {
					audioBytes <- audioToSend
				}

			}
		}
	}
}

//
// getDialogflowTimeoutResult process the two types of timeout errors and return partial results
// type 1: some speech transcribed, but DF timeout before we get any intent-result,
//         return intent_system_timeout
//
// type 2: no speech is detected in the audio,
//         return intent_system_noaudio
//
func (s *Server) getDialogflowTimeoutResult(
	req chipperRequest,
	speechText string,
	speechConfidence float32) (*pb.IntentResponse, string) {

	r := &pb.IntentResponse{
		Session:  req.session,
		DeviceId: req.device,
		Mode:     req.mode,
		IsFinal:  true,
	}

	chippermetrics.CountMatchFail.Inc(1)
	chippermetrics.CountDFMatchFail.Inc(1)

	status := ""
	if speechText != "" {
		// type 1: some speech is transcribed, but no end-of-speech detected before timeout
		// returns intent_system_timeout + partial speech transcript

		chippermetrics.CountTimeout.Inc(1)
		chippermetrics.CountDFTimeout.Inc(1)

		if s.saveAudio && req.firstReq != nil && req.firstReq.SaveAudio {
			r.AudioId = audiostore.BlobId(req.device, req.session, req.audioCodec.String(), GOOG, req.reqTime)
		}

		status = "deadline_exceeded_timeout"
		r.IntentResult = &pb.IntentResult{
			QueryText:        speechText,
			SpeechConfidence: speechConfidence,
			Action:           TimeoutIntent,
			Service:          pb.IntentService_DIALOGFLOW,
		}

	} else {
		// type 2: no speech transcribed, probably streaming silence
		// returns intent_system_noaudio, no transcript

		chippermetrics.CountNoAudio.Inc(1)
		chippermetrics.CountDFTimeoutNoAudio.Inc(1)

		status = "deadline_exceeded_noaudio"
		r.IntentResult = &pb.IntentResult{
			QueryText: "",
			Action:    NoAudioIntent,
			Service:   pb.IntentService_DIALOGFLOW,
		}

	}

	intentLog := alog.Info{
		"action":       "parse_intent_result",
		"status":       status,
		"svc":          GOOG,
		"device":       req.device,
		"session":      req.session,
		"audio_id":     r.AudioId,
		"final_intent": r.IntentResult.Action,
		"speech_conf":  r.IntentResult.SpeechConfidence,
		"encoding":     req.audioCodec,
		"lang":         req.langString,
		"mode":         r.Mode,
		"fw_version":   req.version.FwVersion,
	}
	if s.logTranscript {
		intentLog["query"] = speechText
		intentLog["speech_conf"] = speechConfidence
	}
	intentLog.LogC(req.ctx)

	return r, ""
}

//
// getDialogflowResult process regular intent-result
//
func (s *Server) getDialogflowResult(
	req chipperRequest,
	result *df.IntentResponse,
	speechText string,
	speechConfidence float32) (*pb.IntentResponse, string) {

	var finalIntent string
	if result.QueryText == "" && result.Action == "" {
		// When no speech is detected,
		// Dialogflow returns empty strings for transcript and intent-name
		// chipper will return a special no-audio intent to the client
		finalIntent = NoAudioIntent
		chippermetrics.CountNoAudio.Inc(1)
		chippermetrics.CountDFNoAudio.Inc(1)
		chippermetrics.CountMatchFail.Inc(1)
		chippermetrics.CountDFMatchFail.Inc(1)

		alog.Info{
			"action":     "log_no_results",
			"svc":        GOOG,
			"device":     req.device,
			"session":    req.session,
			"raw_result": result,
		}.LogC(req.ctx)

	} else if result.Action != FallbackIntent {
		finalIntent = result.Action
		chippermetrics.CountMatch.Inc(1)
		chippermetrics.CountDFMatch.Inc(1)
	} else {
		finalIntent = FallbackIntent
		chippermetrics.CountMatchFail.Inc(1)
		chippermetrics.CountDFMatchFail.Inc(1)
	}

	// parse parameters
	var parameters map[string]string = nil
	if ExtendIntent(result.Action) {
		chipperFuncResults := getParameters(
			req.ctx,
			s.chipperfnClient,
			GOOG,
			result.Action,
			result.QueryText,
			nil, // lex
			result.Parameters,
			s.logTranscript,
		)
		if chipperFuncResults != nil {
			if chipperFuncResults.NewIntent != "" {
				finalIntent = chipperFuncResults.NewIntent
			}
			parameters = chipperFuncResults.Parameters
		}
	}

	r := &pb.IntentResponse{
		Session:  req.session,
		DeviceId: req.device,
		Mode:     req.mode,
		IsFinal:  true,
		IntentResult: &pb.IntentResult{
			QueryText:            result.QueryText,
			Action:               finalIntent,
			IntentConfidence:     result.IntentConfidence,
			SpeechConfidence:     speechConfidence,
			Parameters:           parameters,
			AllParametersPresent: result.AllParametersPresent,
			Service:              pb.IntentService_DIALOGFLOW,
		},
	}

	if s.saveAudio && req.firstReq != nil && req.firstReq.SaveAudio {
		r.AudioId = audiostore.BlobId(req.device, req.session, req.audioCodec.String(), GOOG, req.reqTime)
	}

	intentLog := alog.Info{
		"action":       "parse_intent_result",
		"status":       "is_final",
		"svc":          GOOG,
		"device":       req.device,
		"session":      req.session,
		"lang":         req.langString,
		"query_intent": result.Action,
		"final_intent": finalIntent,
		"speech_conf":  result.SpeechConfidence,
		"confidence":   result.IntentConfidence,
		"audio_id":     r.AudioId,
		"mode":         r.Mode,
		"encoding":     req.audioCodec,
		"fw_version":   req.version.FwVersion,
	}

	if s.logTranscript {
		intentLog["query"] = result.QueryText
		intentLog["params"] = parameters
	}
	intentLog.LogC(req.ctx)

	pString := dialogflowParamsString(result.Parameters, s.logTranscript)
	return r, pString
}

// DialogflowTextIntent send text to dialogflow and get matching intent back
func (s *Server) DialogflowTextIntent(
	c df.SessionClient,
	text string,
	req chipperRequest) *chipperResult {

	resp, err := c.DetectTextIntent(req.ctx, text, req.langString)
	if err != nil {
		logDialogflowResponseError(err, s.dialogflowProject, req, df.QueryTypeText)
		return &chipperResult{err: err}
	}

	res, pString := s.getDialogflowResult(req, resp, "", 0.0)

	if s.logTranscript {
		alog.Debug{"dialogflow_param": res.IntentResult.Parameters}.Log()
	}

	return &chipperResult{
		intent:    res,
		rawParams: pString,
		err:       nil,
	}
}

// logDialogflowResponseError outputs Error for PermissionDenied, or Warn for all other errors
func logDialogflowResponseError(err error, conf DialogflowConfig, req chipperRequest, reqType df.QueryType) {
	statusCode := grpc.Code(err)

	if statusCode == codes.PermissionDenied {
		// Alert needed
		creds := dialogflowCred{}
		jErr := json.Unmarshal([]byte(*conf.Creds), &creds)
		if jErr != nil {
			alog.Warn{
				"action": "check_dialogflow_creds",
				"status": alog.StatusFail,
				"error":  jErr,
				"msg":    "fail to unmarshal dialogflow credentials",
			}.LogC(req.ctx)
		}

		alog.Error{
			"action":            "process_dialogflow_error",
			"status":            "dialogflow_permission_denied",
			"config_proj_id":    *conf.ProjectName,
			"cred_proj_id":      creds.ProjId,
			"cred_client_email": creds.ClientEmail,
			"req_type":          reqType,
			"svc":               GOOG,
			"device":            req.device,
			"session":           req.session,
			"error":             err,
		}.LogC(req.ctx)
		chippermetrics.CountDFErrors.Inc(1)

	} else {
		// non-actionable error, just log a warning
		alog.Warn{
			"action":   "process_dialogflow_error",
			"status":   statusCode.String(),
			"req_type": reqType,
			"svc":      GOOG,
			"device":   req.device,
			"session":  req.session,
			"error":    err,
		}.LogC(req.ctx)
		chippermetrics.CountDFRecvErrors.Inc(1)
	}
}

// parameterString converts Dialogflow Parameters into a JSON string (used for Hypothesis Store)
func dialogflowParamsString(parameters *google_protobuf4.Struct, logParams bool) string {
	pJson, err := json.Marshal(parameters)
	if err != nil {
		warnLog := alog.Warn{
			"action": "marshal_dialogflow_params",
			"status": alog.StatusFail,
			"error":  err,
			"msg":    "fail to convert dialogflow params to json string",
		}
		if logParams {
			warnLog["params"] = parameters.String()
		}
		warnLog.Log()
		return ""
	}
	pString := string(pJson)
	if logParams {
		alog.Debug{"Parameter_string": pString}.Log()
	}

	return pString
}
