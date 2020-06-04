package chipper

import (
	"bytes"
	"encoding/json"
	"io"
	"strings"
	"time"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials/stscreds"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/lexruntimeservice"

	"github.com/anki/opus-go/opus"
	ankivad "github.com/anki/sai-go-anki-vad/vad"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/metricsutil"

	"github.com/anki/sai-chipper-voice/audiostore"
	df "github.com/anki/sai-chipper-voice/conversation/dialogflow"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	LEX                 = "lex"
	acceptHeader        = "text/plain; charset=utf-8"
	defaultContentType  = "audio/l16; rate=16000; channels=1"
	compressContentType = "audio/x-cbr-opus-with-preamble; preamble-size=0; bit-rate=67584; frame-size-milliseconds=60"
	farfieldKey         = "x-amz-lex:audio-profile"
	farfieldValue       = "FAR_FIELD"
	lexVadTimeout       = 8 * time.Second
)

type LexConfig struct {
	Enable        bool
	BotName       *string
	BotAlias      *string
	Region        *string
	AssumeRoleArn *string
}

type LexClient struct {
	Config              LexConfig
	GamesConfig         LexConfig
	AwsConfig           aws.Config
	Compress            bool
	Accept              string
	ContentType         string
	SessionName         string
	Session             *session.Session
	EnableFarFieldModel bool
}

func NewLexClient(conf, gamesConf LexConfig, compress, enableFarFieldModel bool, sessionName string) (c *LexClient) {
	c = &LexClient{
		Config:              conf,
		GamesConfig:         gamesConf,
		Compress:            compress,
		Accept:              acceptHeader,
		ContentType:         defaultContentType,
		SessionName:         sessionName,
		EnableFarFieldModel: enableFarFieldModel,
	}

	if compress {
		c.ContentType = compressContentType
	}

	sess := session.Must(session.NewSession())
	c.AwsConfig = *aws.NewConfig().WithRegion(*conf.Region)

	// Chipper Production have direct acccess to Lex, AssumeRoleArn will not be set.
	// Non-prod env will need to assume role to access the Lex bot in the cloudai account.
	if conf.AssumeRoleArn != nil && strings.Contains(*conf.AssumeRoleArn, "arn:aws:iam") {
		creds := stscreds.NewCredentials(sess, *conf.AssumeRoleArn)
		c.AwsConfig.WithCredentials(creds)
	}
	c.Session = sess
	return
}

func (s *Server) processRequestLex(req chipperRequest) *chipperResult {

	chippermetrics.CountLex.Inc(1)

	if s.lexClient == nil {
		alog.Error{"action": "process_lex_request",
			"status": alog.StatusError,
			"error":  "Lex client is nil!",
		}.Log()
		chippermetrics.CountLexErrors.Inc(1)
		return &chipperResult{
			intent: nil,
			err:    status.Errorf(codes.Unimplemented, "Lex client is missing"),
		}
	}

	lex := lexruntimeservice.New(s.lexClient.Session, &s.lexClient.AwsConfig)
	metricsutil.AddAWSMonitoringHandlers(lex.Client)
	userId := createSessionHash(req.device, req.session)

	botName := req.version.IntentVersion.LexBotName
	botAlias := req.version.IntentVersion.LexBotAlias

	pr, pw := io.Pipe()
	postContentInput := lexruntimeservice.PostContentInput{
		Accept:      &s.lexClient.Accept,
		BotAlias:    &botAlias,
		BotName:     &botName,
		ContentType: &s.lexClient.ContentType,
		InputStream: aws.ReadSeekCloser(pr), // all aws need ReadSeekCloser, that's why we cast it
		UserId:      &userId,
	}

	// use Lex secret far-field model for ASR
	// AWS needs to be white-listed to use this feature
	if s.lexClient.EnableFarFieldModel {
		requestAttributes := aws.JSONValue{}
		requestAttributes[farfieldKey] = farfieldValue
		postContentInput.RequestAttributes = requestAttributes
	}

	// send subsequent audio
	saveAudioBytes := make(chan []byte, MaxAudioSamples)
	audioLength := make(chan time.Duration, 1)

	var oggDecoderStream *opus.OggStream
	// setup decompression if required
	if req.audioCodec == df.OggEncoding {
		oggDecoderStream = &opus.OggStream{}
	}

	// send audio to pipe
	go sendAudioLex(
		pw,
		req,
		s.saveAudio, saveAudioBytes,
		audioLength,
		oggDecoderStream,
	)

	// process responses
	output, err := lex.PostContentWithContext(req.ctx, &postContentInput)
	audioDuration := <-audioLength

	// compute intent-latency = (time since result returned) - audioDuration
	latencyDuration := time.Since(req.reqTime) - audioDuration
	chippermetrics.LatencyLex.Update(latencyDuration)
	intentLatencyMs := float32(latencyDuration) / float32(time.Millisecond)

	if err != nil {
		alog.Warn{"action": "send_audio",
			"status":  alog.StatusError,
			"svc":     LEX,
			"device":  req.device,
			"session": req.session,
			"msg":     "fail to send audio to Lex PostContent operation",
		}.LogC(req.ctx)

		chippermetrics.CountLexPostContentErrors.Inc(1)

		return &chipperResult{
			intent:      nil,
			audioLength: &audioDuration,
			err:         status.Errorf(codes.Internal, "Lex PostContent error %s", err.Error()),
		}
	}

	var audioBytes bytes.Buffer
	if s.saveAudio && len(saveAudioBytes) > 0 {
		remaining := flushAudio(saveAudioBytes, req, LEX)
		audioBytes.Write(remaining.Bytes())
	}

	r, paramsString := s.getLexResult(req, output, s.saveAudio)

	err = req.stream.Send(r)

	if err != nil {
		logSendClientError(req, LEX, SendClientErrorMsg, err, nil, intentLatencyMs)
		return &chipperResult{
			intent:      nil,
			audioLength: &audioDuration,
			err:         err,
		}
	}

	alog.Info{
		"action":            "send_client",
		"status":            alog.StatusOK,
		"svc":               LEX,
		"device":            req.device,
		"session":           req.session,
		"intent":            r.IntentResult.Action,
		"msg":               "sent lex results to client",
		"intent_latency_ms": intentLatencyMs,
	}.LogC(req.ctx)

	chippermetrics.CountStreamResponse.Inc(1)

	return &chipperResult{
		intent:      r,
		audio:       &audioBytes,
		rawParams:   paramsString,
		audioLength: &audioDuration,
		err:         nil,
	}
}

func (s *Server) getLexResult(
	req chipperRequest,
	output *lexruntimeservice.PostContentOutput,
	saveAudio bool) (*pb.IntentResponse, string) {

	r := new(pb.IntentResponse)
	r.Session = req.session
	r.DeviceId = req.device

	if s.logTranscript {
		alog.Debug{"lex_response": output}.Log()
	}

	transcript := ""
	if output.InputTranscript != nil {
		transcript = *output.InputTranscript
	}

	finalIntent := FallbackIntent
	if output.IntentName != nil {
		finalIntent = *output.IntentName
		chippermetrics.CountMatch.Inc(1)
		chippermetrics.CountLexMatch.Inc(1)
	} else {
		chippermetrics.CountMatchFail.Inc(1)
		chippermetrics.CountLexMatchFail.Inc(1)
	}

	if transcript == "" && finalIntent == FallbackIntent {
		// no audio
		finalIntent = NoAudioIntent
	}

	// parse slots
	var parameters map[string]string = nil

	if ExtendIntent(finalIntent) {
		chipperFuncResults := getParameters(
			req.ctx,
			s.chipperfnClient,
			LEX,
			finalIntent,
			transcript,
			output.Slots,
			nil, // for google
			s.logTranscript,
		)
		if chipperFuncResults != nil {
			if chipperFuncResults.NewIntent != "" {
				finalIntent = chipperFuncResults.NewIntent
			}
			parameters = chipperFuncResults.Parameters
		}
	}

	r.IntentResult = &pb.IntentResult{
		QueryText:  transcript,
		Action:     finalIntent,
		Parameters: parameters,
		Service:    pb.IntentService_LEX,
	}
	r.IsFinal = true

	if saveAudio && req.firstReq != nil && req.firstReq.SaveAudio {
		r.AudioId = audiostore.BlobId(req.device, req.session, req.audioCodec.String(), LEX, req.reqTime)
	}

	intentLog := alog.Info{
		"action":       "parse_intent_result",
		"status":       "is_final",
		"svc":          LEX,
		"device":       req.device,
		"session":      req.session,
		"lang":         req.langString,
		"final_intent": finalIntent,
		"audio_id":     r.AudioId,
		"mode":         req.mode,
		"encoding":     req.audioCodec,
		"fw_version":   req.version.FwVersion,
	}
	if s.logTranscript {
		intentLog["query"] = transcript
		intentLog["params"] = r.IntentResult.Parameters
	}
	intentLog.LogC(req.ctx)

	return r, lexParamsString(output.Slots, s.logTranscript)
}

func sendAudioLex(
	pw *io.PipeWriter,
	reqInfo chipperRequest,
	saveAudio bool,
	savedBytes chan []byte,
	audioLength chan time.Duration,
	oggDecoderStream *opus.OggStream) {

	firstAudioBytes := reqInfo.firstReq.InputAudio
	if oggDecoderStream != nil {
		decoded, err := oggDecoderStream.Decode(firstAudioBytes)
		if err != nil {
			alog.Warn{
				"action":  "decode_audio",
				"status":  alog.StatusFail,
				"svc":     LEX,
				"error":   err,
				"device":  reqInfo.device,
				"session": reqInfo.session,
				"msg":     "fail to decode opus audio for lex, exiting",
			}.LogC(reqInfo.ctx)
			chippermetrics.CountLexErrors.Inc(1)
			return
		}
		firstAudioBytes = decoded
	}
	pw.Write(firstAudioBytes)

	var audioBytes bytes.Buffer = *bytes.NewBuffer(firstAudioBytes)

	vad, err := ankivad.NewAnkiVad()
	if err != nil {
		alog.Warn{
			"action":  "create_anki_vad",
			"status":  alog.StatusError,
			"error":   err,
			"svc":     LEX,
			"device":  reqInfo.device,
			"session": reqInfo.session,
			"msg":     "fail to create new Anki Vad",
		}.LogC(reqInfo.ctx)
		pw.Close()
		audioLength <- time.Since(reqInfo.reqTime)
		return
	}
	defer vad.CleanUp()

	vadTimer := time.NewTimer(lexVadTimeout)
	defer vadTimer.Stop()

	sentBytes := len(firstAudioBytes)
	totalBytes := len(reqInfo.firstReq.InputAudio)
	frame := 1
	elapsed := audioElapsedMs{float32(0), float32(0), float32(0), float32(0)}

	for {
		req, err := reqInfo.stream.Recv()

		if err != nil {
			pw.Close()
			savedBytes <- audioBytes.Bytes()
			audioLength <- time.Since(reqInfo.reqTime)
			logRecvAudioSummary(reqInfo, LEX, frame, totalBytes, sentBytes, elapsed)
			logRecvAudioErrors(reqInfo, LEX, err)

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

		audioToSend := req.GetInputAudio()
		if oggDecoderStream != nil {
			decoded, err := oggDecoderStream.Decode(audioToSend)
			if err != nil {
				alog.Warn{"action": "decode_audio",
					"status":  alog.StatusFail,
					"svc":     LEX,
					"error":   err,
					"device":  reqInfo.device,
					"session": reqInfo.session,
					"msg":     "fail to decode opus audio for lex, exiting",
				}.LogC(reqInfo.ctx)
				chippermetrics.CountLexErrors.Inc(1)
				audioLength <- time.Since(reqInfo.reqTime)
				logRecvAudioSummary(reqInfo, LEX, frame, totalBytes, sentBytes, elapsed)

				return
			}
			audioToSend = decoded
		}

		pw.Write(audioToSend)
		sentBytes += len(audioToSend)
		elapsed.sent = elapsed.total
		if saveAudio {
			audioBytes.Write(audioToSend)
		}

		end, err := vad.CheckForSpeechEnd(audioToSend)
		if err != nil {
			alog.Warn{
				"action":           "check_vad",
				"status":           alog.StatusError,
				"error":            err,
				"svc":              LEX,
				"device":           reqInfo.device,
				"session":          reqInfo.session,
				"msg":              "vad error",
				"send_duration_ms": durationMs(reqInfo.reqTime),
			}.LogC(reqInfo.ctx)
			pw.Close()
			savedBytes <- audioBytes.Bytes()
			audioLength <- time.Since(reqInfo.reqTime)
			chippermetrics.CountLexVadErrors.Inc(1)
			logRecvAudioSummary(reqInfo, LEX, frame, totalBytes, sentBytes, elapsed)

			return
		}

		if end {
			pw.Close()
			alog.Debug{
				"action":           "check_vad",
				"status":           "end_of_speech",
				"svc":              LEX,
				"device":           reqInfo.device,
				"session":          reqInfo.session,
				"msg":              "end_of_speech detected, close pw and exit",
				"send_duration_ms": durationMs(reqInfo.reqTime),
			}.LogC(reqInfo.ctx)
			savedBytes <- audioBytes.Bytes()
			audioLength <- time.Since(reqInfo.reqTime)
			logRecvAudioSummary(reqInfo, LEX, frame, totalBytes, sentBytes, elapsed)

			return
		}

		// set a timeout
		select {
		case <-vadTimer.C:
			pw.Close()
			alog.Warn{
				"action":           "recv_audio",
				"status":           "timeout",
				"svc":              LEX,
				"device":           reqInfo.device,
				"session":          reqInfo.session,
				"msg":              "vad timeout, close pw and exit",
				"send_duration_ms": durationMs(reqInfo.reqTime),
			}.LogC(reqInfo.ctx)
			savedBytes <- audioBytes.Bytes()
			audioLength <- time.Since(reqInfo.reqTime)
			chippermetrics.CountLexVadTimeout.Inc(1)
			logRecvAudioSummary(reqInfo, LEX, frame, totalBytes, sentBytes, elapsed)

			return
		default:
			continue
		}
	}
}

func (s *Server) PerformLexTextRequest(text string, req chipperRequest) *chipperResult {

	if s.lexClient == nil {
		alog.Error{"action": "process_lex_text_request",
			"status": alog.StatusError,
			"error":  "Lex client is nil!",
		}.Log()
		return &chipperResult{
			intent: nil,
			err:    status.Errorf(codes.Unimplemented, "Lex client is missing"),
		}
	}

	userId := createSessionHash(req.device, req.session)

	newSession, _ := session.NewSession()
	svc := lexruntimeservice.New(newSession, &s.lexClient.AwsConfig)

	botName := req.version.IntentVersion.LexBotName
	botAlias := req.version.IntentVersion.LexBotAlias

	postTextInput := lexruntimeservice.PostTextInput{
		BotAlias:  &botAlias,
		BotName:   &botName,
		InputText: &text,
		UserId:    &userId,
	}

	output, err := svc.PostText(&postTextInput)
	if err != nil {
		alog.Warn{
			"action":  "text_request",
			"status":  alog.StatusError,
			"error":   err,
			"svc":     LEX,
			"device":  req.device,
			"session": req.session,
		}.LogC(req.ctx)
		return &chipperResult{
			intent: nil,
			err:    status.Errorf(codes.Internal, "lex PostText error %s", err.Error()),
		}
	}

	if s.logTranscript {
		alog.Info{
			"action":  "text_request",
			"status":  alog.StatusOK,
			"svc":     LEX,
			"device":  req.device,
			"session": req.session,
			"output":  output,
		}.LogC(req.ctx)
	}

	r := new(pb.IntentResponse)
	r.Session = req.session
	r.DeviceId = req.device

	finalIntent := FallbackIntent
	if output.IntentName != nil {
		finalIntent = *output.IntentName
	}

	// parse slots
	var parameters map[string]string = nil

	if ExtendIntent(finalIntent) {
		chipperFuncResults := getParameters(
			req.ctx,
			s.chipperfnClient,
			LEX,
			finalIntent,
			text,
			output.Slots,
			nil, // for google
			s.logTranscript,
		)
		if chipperFuncResults != nil {
			if chipperFuncResults.NewIntent != "" {
				finalIntent = chipperFuncResults.NewIntent
			}
			parameters = chipperFuncResults.Parameters
		}
	}

	r.IntentResult = &pb.IntentResult{
		QueryText:  text,
		Action:     finalIntent,
		Parameters: parameters,
		Service:    pb.IntentService_LEX,
	}
	r.IsFinal = true

	intentLog := alog.Info{
		"action":       "parse_intent_result",
		"status":       "is_final",
		"svc":          LEX,
		"device":       r.DeviceId,
		"session":      r.Session,
		"final_intent": r.IntentResult.Action,
		"mode":         req.mode,
		"lang":         req.langString,
		"fw_version":   req.version.FwVersion,
	}
	if s.logTranscript {
		intentLog["query"] = r.IntentResult.QueryText
		intentLog["params"] = r.IntentResult.Parameters
	}
	intentLog.LogC(req.ctx)

	return &chipperResult{
		intent:    r,
		rawParams: lexParamsString(output.Slots, s.logTranscript),
		err:       nil,
	}
}

// lexParamsString converts Lex output slots (entity) to a Json string
func lexParamsString(entity interface{}, logParams bool) (pString string) {
	pJson, err := json.Marshal(entity)
	if err != nil {
		warnLog := alog.Warn{
			"action": "marshal_lex_entities",
			"status": "fail",
			"error":  err,
		}
		if logParams {
			warnLog["params"] = entity
		}
		warnLog.Log()
		pString = ""
	} else {
		pString = string(pJson)
	}
	return
}
