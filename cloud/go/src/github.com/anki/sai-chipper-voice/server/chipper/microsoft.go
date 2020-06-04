package chipper

import (
	"bytes"
	"io"
	"strings"
	"time"

	"github.com/satori/go.uuid"

	"github.com/anki/sai-go-util/log"

	"encoding/json"
	"github.com/anki/sai-chipper-voice/audiostore"
	"github.com/anki/sai-chipper-voice/conversation/microsoft"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/server/chipperfunction"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	MS                = "msft"
	SpeechOnlyIntent  = "intent_speech_only"
	bingSpeechTimeout = 8 * time.Second
)

type MicrosoftConfig struct {
	// Microsoft
	BingKey   *string
	LuisKey   *string
	LuisAppId *string
}

// Microsoft Bing and Luis
func (s *Server) processRequestMicrosoft(req chipperRequest) *chipperResult {

	chippermetrics.CountMS.Inc(1)

	newUUID, _ := uuid.NewV4()
	microsoftSession := strings.Replace(newUUID.String(), "-", "", -1)

	config := &microsoft.MicrosoftConfiguration{
		BingKey:      *s.microsoftConfig.BingKey,
		LanguageCode: req.langString,
		RequestId:    microsoftSession,
	}

	msClient, err := microsoft.NewMicrosoftClient(config)
	if err != nil {
		alog.Info{
			"action":  "create_ms_client",
			"status":  alog.StatusFail,
			"error":   err,
			"svc":     MS,
			"device":  req.device,
			"session": req.session,
		}.LogC(req.ctx)
		chippermetrics.CountMSErrors.Inc(1)
		return &chipperResult{err: err}
	}

	// send first packet
	header := msClient.GetSendHeader(microsoftSession)

	msClient.SendAudio(req.firstReq.InputAudio, header)

	// POC-debug
	alog.Info{
		"action": "send_first_audio",
		"svc":    MS,
		"device": req.device, "session": req.session,
		"service":    pb.IntentService_BING_LUIS,
		"time_since": durationMs(req.reqTime),
	}.LogC(req.ctx)

	done := make(chan struct{}, 1)

	// send audio to Bing to get transcript
	waitc := make(chan struct{})
	saveAudioBytes := make(chan []byte, MaxAudioSamples)
	audioLength := make(chan time.Duration, 1)

	// dev only - save first audio packet
	var audioBytes bytes.Buffer
	if s.saveAudio {
		audioBytes.Write(req.firstReq.InputAudio)
	}

	go sendAudioMicrosoft(msClient,
		req,
		header,
		done,
		waitc,
		s.saveAudio,
		saveAudioBytes,
		audioLength,
	)

	// receive results
	transcript := ""
	var res *pb.IntentResponse = nil
	paramsString := ""
microsoftLoop:
	for {
		select {
		case audio := <-saveAudioBytes:
			if s.saveAudio {
				audioBytes.Write(audio)
			}
		default:
			// get audio result
			hypothesis, isFinal, err := msClient.RecvHypothesis()
			if err != nil {
				done <- struct{}{}
				chippermetrics.CountMSErrors.Inc(1)
				break microsoftLoop
			}

			if hypothesis != "" && !isFinal {
				transcript = hypothesis
			}

			if isFinal {
				done <- struct{}{}

				// get intent result
				var intent *microsoft.LuisResult
				if !req.asr {
					intent, err = s.luisClient.GetIntent(transcript)
					if err != nil {
						chippermetrics.CountMSErrors.Inc(1)
						break microsoftLoop
					}
				} else {

					intent = &microsoft.LuisResult{
						Transcript: transcript,
						Intent:     SpeechOnlyIntent,
						Score:      float32(1),
					}

				}

				if intent != nil {
					res, paramsString = s.getLuisResult(req, intent)

					err = req.stream.Send(res)
					chippermetrics.CountStreamResponse.Inc(1)

					if err != nil {
						alog.Warn{
							"action":  "send_client_resp",
							"svc":     MS,
							"status":  "fail",
							"error":   err,
							"device":  req.device,
							"session": req.session,
						}.LogC(req.ctx)
						chippermetrics.CountStreamResponseError.Inc(1)
					}

					break microsoftLoop
				}
			}
		}
	}
	<-waitc // wait for sendAudio goroutine to close
	if s.saveAudio && len(saveAudioBytes) > 0 {
		remaining := flushAudio(saveAudioBytes, req, MS)
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

func sendAudioMicrosoft(msClient *microsoft.MicrosoftClient,
	reqInfo chipperRequest,
	header []byte,
	done, waitc chan struct{},
	saveAudio bool,
	audioBytes chan []byte,
	audioLength chan time.Duration) {

	defer close(waitc)

	// timeout for Bing Speech
	audioTimer := time.NewTimer(bingSpeechTimeout)
	defer audioTimer.Stop()

	totalBytes := len(reqInfo.firstReq.InputAudio)
	stopSending := false
	frame := 1
	for {
		select {
		case <-audioTimer.C:
			alog.Warn{
				"action":  "recv_audio",
				"status":  "timeout",
				"svc":     MS,
				"device":  reqInfo.device,
				"session": reqInfo.session,
			}.LogC(reqInfo.ctx)
			audioLength <- time.Since(reqInfo.reqTime)
			return

		case <-done:
			stopSending = true
			alog.Info{
				"action":  "recv_done_signal",
				"svc":     MS,
				"device":  reqInfo.device,
				"session": reqInfo.session,
			}.LogC(reqInfo.ctx)
			audioLength <- time.Since(reqInfo.reqTime)
			return

		default:
			req, err := reqInfo.stream.Recv()
			if err == io.EOF {
				alog.Info{
					"action": "recv_audio",
					"svc":    MS,
					"status": "client_stop_streaming",
					"error":  "io.EOF",
				}.LogC(reqInfo.ctx)
				audioLength <- time.Since(reqInfo.reqTime)
				return
			}

			if err != nil {
				alog.Warn{
					"action": "recv_audio",
					"svc":    MS,
					"status": "unexpected_error",
					"error":  err,
				}.LogC(reqInfo.ctx)
				checkRequestError(err)
				audioLength <- time.Since(reqInfo.reqTime)
				return
			}

			// POC-debug
			frame++
			totalBytes += len(req.InputAudio)
			alog.Info{
				"action":          "recv_audio",
				"status":          alog.StatusOK,
				"svc":             MS,
				"frame":           frame,
				"encoding":        reqInfo.firstReq.AudioEncoding,
				"bytes_recv":      len(req.InputAudio),
				"time_elapsed_ms": durationMs(reqInfo.reqTime),
				"rate":            computeRate(totalBytes, time.Since(reqInfo.reqTime), reqInfo.firstReq.AudioEncoding),
				"device":          reqInfo.device,
				"session":         reqInfo.session,
			}.LogC(reqInfo.ctx)

			if !stopSending {
				err = msClient.SendAudio(req.InputAudio, header)
				if saveAudio {
					audioBytes <- req.InputAudio
				}
			}

		}
	}
}

func (s *Server) getLuisResult(req chipperRequest, intent *microsoft.LuisResult) (*pb.IntentResponse, string) {
	// luis always return FallbackIntent if for some reason, intent.Score is too low
	var finalIntent string
	if intent.Intent != FallbackIntent {
		finalIntent = intent.Intent
		chippermetrics.CountMatch.Inc(1)
		chippermetrics.CountMSMatch.Inc(1)
	} else {
		finalIntent = FallbackIntent
		if intent.Transcript == "" {
			finalIntent = NoAudioIntent
			chippermetrics.CountNoAudio.Inc(1)
		}
		chippermetrics.CountMatchFail.Inc(1)
		chippermetrics.CountMSMatchFail.Inc(1)

	}

	var audioId string
	audioId = ""
	if s.saveAudio && (req.firstReq != nil && req.firstReq.SaveAudio) {
		audioId = audiostore.BlobId(req.device, req.session, req.audioCodec.String(), MS, req.reqTime)
	}

	chipperfuncResult := chipperfunction.ParseLuis(finalIntent, intent.Transcript, intent.Blob)
	if chipperfuncResult.NewIntent != "" {
		finalIntent = chipperfuncResult.NewIntent
	}

	intentLog := alog.Info{
		"action":       "parse_intent_result",
		"status":       "is_final",
		"svc":          MS,
		"device":       req.device,
		"session":      req.session,
		"query_intent": intent.Intent,
		"final_intent": finalIntent,
		"confidence":   intent.Score,
		"audio_id":     audioId,
		"lang":         req.langString,
		"encoding":     req.audioCodec,
		"mode":         req.mode,
		"fw_version":   req.version.FwVersion,
	}
	if s.logTranscript {
		intentLog["query"] = intent.Transcript
	}
	intentLog.LogC(req.ctx)

	// convert entity to a JSON string for Hypothesis Store
	pJson, err := json.Marshal(intent.Entities)
	pString := ""
	if err != nil {
		errLog := alog.Warn{
			"action": "marshal_luis_entities",
			"status": alog.StatusError,
			"error":  err,
			"msg":    "fail to convert luis entites to json",
		}
		if s.logTranscript {
			errLog["entity"] = intent.Entities
		}
		errLog.Log()
	} else {
		pString = string(pJson)
	}

	return &pb.IntentResponse{
		Session:  req.session,
		DeviceId: req.device,
		IsFinal:  true,
		AudioId:  audioId,
		IntentResult: &pb.IntentResult{
			QueryText:            intent.Transcript,
			Action:               finalIntent,
			IntentConfidence:     intent.Score,
			Parameters:           chipperfuncResult.Parameters,
			AllParametersPresent: true,
			Service:              pb.IntentService_BING_LUIS},
	}, pString
}
