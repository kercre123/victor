package chipper

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"io"
	"time"

	"github.com/youpy/go-wav"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/anki/houndify-sdk-go/houndify"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"

	"github.com/anki/sai-chipper-voice/audiostore"
	"github.com/anki/sai-chipper-voice/conversation/dialogflow"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	HOUNDIFY              = "houndify"
	houndifyClientTimeout = 8 * time.Second
	NO_RESULT             = "NoResultCommand"
	NO_RESULT_SPOKEN      = "Didn't Get That"
)

type HoundifyConfig struct {
	ClientId  *string
	ClientKey *string
}

type HoundifyVoiceActivityDetection struct {
	MaxSilenceSeconds                  float64 `json:"MaxSilenceSeconds"`
	MaxSilenceAfterFullQuerySeconds    float64 `json:"MaxSilenceAfterFullQuerySeconds"`
	MaxSilenceAfterPartialQuerySeconds float64 `json:"MaxSilenceAfterPartialQuerySeconds"`
}

func NewHoundifyClient(config HoundifyConfig) (c *houndify.Client) {
	return &houndify.Client{
		ClientID:  *config.ClientId,
		ClientKey: *config.ClientKey,
	}
}

func (s *Server) processRequestHoundify(req kgRequest) *chipperResult { // (*pb.KnowledgeGraphResponse, *bytes.Buffer, error) {
	chippermetrics.CountHoundify.Inc(1)

	userId := createSessionId(req.device, req.session)

	reqId, err := strutil.ShortLowerUUID()
	if err != nil {
		alog.Warn{
			"action":  "create_request_id",
			"status":  alog.StatusError,
			"svc":     HOUNDIFY,
			"error":   err,
			"device":  req.device,
			"session": req.session,
		}.LogC(req.stream.Context())
		chippermetrics.CountHoundifyErrors.Inc(1)
		return &chipperResult{
			err: status.Errorf(codes.Aborted, "fail to create request-id for houndify"),
		}
	}

	var audioBytes bytes.Buffer
	if s.saveAudio {
		audioBytes = *bytes.NewBuffer(req.firstReq.InputAudio)
	}

	requestInfoFields := make(map[string]interface{})
	requestInfoFields["PartialTranscriptsDesired"] = true

	pr, pw := io.Pipe()
	request := houndify.VoiceRequest{
		AudioStream:       pr,
		UserID:            userId,
		RequestID:         reqId,
		RequestInfoFields: requestInfoFields,
	}

	finishAudioChan := make(chan struct{}, 1)
	finishPartialChan := make(chan struct{}, 1)
	saveAudioBytes := make(chan []byte, MaxAudioSamples)

	go sendAudioHoundify(pw, req, s.saveAudio, saveAudioBytes, finishAudioChan)

	// This channel needs to be buffered enough so that it never blocks, otherwise the houndify sdk will leak goroutines.
	partialTranscripts := make(chan houndify.PartialTranscript, 100)
	go func() {
		for {
			select {
			case partial := <-partialTranscripts:
				if partial.SafeToStopAudio != nil && *partial.SafeToStopAudio == true {
					alog.Info{
						"action":  "houndify_vad",
						"svc":     HOUNDIFY,
						"status":  "houndify_vad_triggered",
						"error":   err,
						"device":  req.device,
						"session": req.session,
						"msg":     "houndify sent back end of speech",
					}.LogC(req.stream.Context())
					chippermetrics.CountHoundifyVadTriggered.Inc(1)
					finishAudioChan <- struct{}{}
					return
				}
			case <-finishPartialChan:
				return
			}
		}
	}()

	ctx, cancel := context.WithTimeout(req.stream.Context(), houndifyClientTimeout)
	defer cancel()
	serverResponse, err := s.houndifyClient.VoiceSearch(ctx, request, partialTranscripts)
	audioDuration := time.Since(req.reqTime)
	finishPartialChan <- struct{}{}

	if s.saveAudio && len(saveAudioBytes) > 0 {
		nChan := len(saveAudioBytes)
		for i := 0; i < nChan; i++ {
			audioBytes.Write(<-saveAudioBytes)
		}
		alog.Debug{
			"action": "flush_kg_audio",
			"frames": nChan,
			"svc":    HOUNDIFY,
		}.LogC(req.stream.Context())
	}

	if err != nil {
		alog.Warn{"action": "voice_search",
			"status":  alog.StatusError,
			"svc":     HOUNDIFY,
			"error":   err,
			"device":  req.device,
			"session": req.session,
			"msg":     "fail to get houndify result",
		}.LogC(req.stream.Context())
		chippermetrics.CountHoundifyErrors.Inc(1)
		return &chipperResult{
			audio: &audioBytes,
			err:   status.Errorf(codes.Internal, "houndify voice-search fail, error %s", err),
		}
	}

	r, raw, err := s.getHoundifyResult(req, serverResponse, s.saveAudio)
	if err != nil {
		alog.Warn{"action": "parse_houndify_response",
			"status":  alog.StatusFail,
			"svc":     HOUNDIFY,
			"error":   err,
			"device":  req.device,
			"session": req.session,
		}.LogC(req.stream.Context())
		chippermetrics.CountHoundifyErrors.Inc(1)
		return &chipperResult{
			audio: &audioBytes,
			err:   status.Errorf(codes.Internal, "houndify get-result fail, error %s", err),
		}
	}

	err = req.stream.Send(r)
	if err != nil {
		alog.Warn{"action": "send_client",
			"status":  alog.StatusFail,
			"error":   err,
			"svc":     HOUNDIFY,
			"device":  req.device,
			"session": req.session,
		}.LogC(req.stream.Context())
		chippermetrics.CountKnowledgeGraphResponseError.Inc(1)
		return &chipperResult{
			audio: &audioBytes,
			err:   err,
		}
	}
	chippermetrics.CountKnowledgeGraphResponse.Inc(1)

	return &chipperResult{
		kg:          r,
		kgResponse:  raw,
		audio:       &audioBytes,
		err:         nil,
		rawParams:   serverResponse,
		audioLength: &audioDuration,
	}
}

func sendAudioHoundify(pw *io.PipeWriter,
	reqInfo kgRequest,
	saveAudio bool,
	savedBytes chan []byte,
	doneChan chan struct{}) {

	if reqInfo.audioCodec == dialogflow.LinearEncoding {
		wav.NewWriter(pw, 0, 1, 16000, 16)
	}

	// we send the ogg-opus audio directly to Houndify
	pw.Write(reqInfo.firstReq.InputAudio)
	totalBytes := len(reqInfo.firstReq.InputAudio)

	timeoutTimer := time.NewTimer(houndifyClientTimeout)
	defer timeoutTimer.Stop()

	frame := 1
	elapsed := audioElapsedMs{float32(0), float32(0), float32(0), float32(0)}

	for {
		req, err := reqInfo.stream.Recv()

		errCode := grpc.Code(err)
		if err == io.EOF || errCode == codes.Canceled || errCode == codes.DeadlineExceeded {
			pw.Close()

			alog.Info{
				"action":     "recv_audio",
				"svc":        HOUNDIFY,
				"status":     "client_stop_streaming",
				"error":      err,
				"error_code": errCode,
				"device":     reqInfo.device,
				"session":    reqInfo.session,
				"msg":        "client stop sending, close pipe and exit",
			}.LogC(reqInfo.stream.Context())

			logKGAudioSummary(reqInfo, HOUNDIFY, frame, totalBytes, elapsed)

			return
		}

		if err != nil {
			chippermetrics.CountKGRecvAudioError.Inc(1)
			pw.Close()

			alog.Warn{
				"action":  "recv_audio",
				"svc":     HOUNDIFY,
				"status":  "unexpected_error",
				"error":   err,
				"device":  reqInfo.device,
				"session": reqInfo.session,
				"msg":     "unexpected error, close pipe and exit",
			}.LogC(reqInfo.stream.Context())

			logKGAudioSummary(reqInfo, HOUNDIFY, frame, totalBytes, elapsed)

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

		pw.Write(req.GetInputAudio())
		if saveAudio {
			savedBytes <- req.InputAudio
		}

		// set a timeout
		select {
		case <-timeoutTimer.C:
			pw.Close()
			chippermetrics.CountKGSendAudioError.Inc(1)

			alog.Warn{
				"action":  "send_audio",
				"status":  "timeout",
				"error":   "houndify request timeout",
				"svc":     HOUNDIFY,
				"device":  reqInfo.device,
				"session": reqInfo.session,
				"msg":     "timeout, close pipe and exit",
			}.LogC(reqInfo.stream.Context())

			logKGAudioSummary(reqInfo, HOUNDIFY, frame, totalBytes, elapsed)
			return
		case <-doneChan:
			pw.Close()
			logKGAudioSummary(reqInfo, HOUNDIFY, frame, totalBytes, elapsed)

			return
		default:
			continue
		}
	}
}

func (s *Server) getHoundifyResult(
	req kgRequest,
	serverResponse string,
	saveAudio bool,
) (*pb.KnowledgeGraphResponse, *houndify.HoundifyResponse, error) {

	r := new(pb.KnowledgeGraphResponse)
	r.Session = req.session
	r.DeviceId = req.device

	alog.Debug{
		"houndify_response": serverResponse,
	}.LogC(req.stream.Context())

	var result houndify.HoundifyResponse
	err := json.Unmarshal([]byte(serverResponse), &result)
	if err != nil {
		return nil, nil, errors.New("failed to decode json")
	}
	if !(result.Status == "OK") {
		return nil, nil, errors.New(*result.ErrorMessage)
	}
	if result.NumToReturn < 1 || len(result.AllResults) < 1 {
		return nil, nil, errors.New("no results to return")
	}

	if result.Disambiguation == nil {
		return nil, nil, errors.New("no Disabiguation listed")
	}
	if result.Disambiguation.NumToShow < 1 || len(result.Disambiguation.ChoiceData) < 1 {
		return nil, nil, errors.New("no Choices listed")
	}

	command := result.AllResults[0]

	if command.UserVisibleMode != nil {
		r.CommandType = NO_RESULT
		r.SpokenText = NO_RESULT_SPOKEN
	} else {
		r.CommandType = command.CommandKind
		r.SpokenText = command.SpokenResponseLong
	}
	r.QueryText = result.Disambiguation.ChoiceData[0].Transcription
	for _, domain := range result.DomainUsage {
		r.DomainsUsed = append(r.DomainsUsed, domain.Domain)
	}

	if r.CommandType != NO_RESULT {
		chippermetrics.CountKnowledgeGraphMatch.Inc(1)
		chippermetrics.CountHoundifyMatch.Inc(1)
	} else {
		chippermetrics.CountKnowledgeGraphMatchFail.Inc(1)
		chippermetrics.CountHoundifyMatchFail.Inc(1)
	}

	if saveAudio && req.firstReq != nil && req.firstReq.SaveAudio {
		r.AudioId = audiostore.BlobId(req.device, req.session, req.audioCodec.String(), HOUNDIFY, req.reqTime)
	}

	logEntry := alog.Info{
		"action":     "parse_knowledge_result",
		"status":     "is_final",
		"svc":        HOUNDIFY,
		"device":     req.device,
		"session":    req.session,
		"command":    r.CommandType,
		"encoding":   req.audioCodec,
		"lang":       req.langString,
		"audio_id":   r.AudioId,
		"fw_version": req.fw,
	}
	if s.logTranscript {
		logEntry["query"] = r.QueryText
	}
	logEntry.LogC(req.stream.Context())

	return r, &result, nil

}

func logKGAudioSummary(req kgRequest, svc string, frames, totalBytes int, elapsed audioElapsedMs) {
	alog.Info{
		"action":           "recv_audio",
		"status":           "summary",
		"svc":              svc,
		"total_frames":     frames,
		"encoding":         req.firstReq.AudioEncoding,
		"total_bytes_recv": totalBytes, // ogg bytes
		"total_bytes_sent": totalBytes, // pcm bytes
		"time_elapsed_ms":  elapsed,
		"device":           req.device,
		"session":          req.session,
	}.LogC(req.stream.Context())
}
