package chipper

import (
	"bytes"
	"context"
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"io"
	"regexp"
	"strings"
	"time"

	google_protobuf4 "github.com/golang/protobuf/ptypes/struct"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/anki/sai-chipper-fn/proto/chipperfnpb"
	"github.com/anki/sai-go-util/log"
	token_model "github.com/anki/sai-token-service/model"

	"github.com/anki/sai-chipper-voice/client/chipperfn"
	"github.com/anki/sai-chipper-voice/conversation/dialogflow"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/server/chipperfunction"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	HypothesisTimeFormat = "2006-01-02-15"
	RobotIdLength        = 8
	EmptyDeviceId        = ""
	TestDeviceIdPrefix   = "TUHQZQ3Na4U9" // some random string
	MaxSessionLength     = 36             // for dialogflow, "the length of the session ID must not exceed 36 bytes"

	SendClientErrorMsg        = "fail to send final result to client"
	SendClientTimeoutErrorMsg = "fail to send dialogflow deadline-exceeded result"
)

type audioElapsedMs struct {
	total   float32
	first10 float32
	first20 float32
	sent    float32
}

func (e audioElapsedMs) ToLogValues() map[string]interface{} {
	return map[string]interface{}{
		"_total": e.total,
		"_10":    e.first10,
		"_20":    e.first20,
		"_sent":  e.sent,
	}
}

//
// Start - Parameter Parsing related
//

var ExtendPattern = regexp.MustCompile(`_extend\b`)

// ExtendIntent checks the intent-name for "_extend" to decide if chipper-fn needs to be called
func ExtendIntent(intentName string) bool {
	return ExtendPattern.MatchString(intentName)
}

// getParameters parse the entities with chipperfn or locally (LEX and GOOG)
// if intent is not implemented in chipperfn, we will get an error, and process the parameters locally
func getParameters(
	ctx context.Context,
	chipperfnClient chipperfn.Client,
	service string,
	intent string,
	text string,
	slots interface{},
	parameters *google_protobuf4.Struct,
	logTranscript bool) *chipperfunction.ChipperFunctionResult {

	if chipperfnClient != nil {
		var req *chipperfnpb.ServerDataIn

		if service == LEX {
			bytes, err := json.Marshal(slots)
			if err != nil {
				alog.Warn{
					"action": "convert_lex_slots_bytes",
					"status": alog.StatusError,
					"error":  err,
					"msg":    "Error mashalling interface map",
				}.LogC(ctx)
			} else {
				req = &chipperfnpb.ServerDataIn{
					IntentService: chipperfnpb.IntentService_LEX,
					IntentName:    intent,
					Text:          text,
					LexParameters: bytes,
				}
			}
		} else if service == GOOG {
			req = &chipperfnpb.ServerDataIn{
				IntentService:        chipperfnpb.IntentService_DIALOGFLOW,
				IntentName:           intent,
				Text:                 text,
				DialogflowParameters: parameters,
			}

		}

		if req != nil {
			r, err := chipperfnClient.Server(ctx, req)

			if err != nil {
				errCode := grpc.Code(err)
				if errCode != codes.Unimplemented {
					// unimplemented errors are expected, only log other unexpected errors
					if errCode == codes.Unknown {
						chippermetrics.CountChipperfnUnknownIntent.Inc(1)
					} else {
						chippermetrics.CountChipperfnFail.Inc(1)
					}

					alog.Warn{
						"action":       "get_parameters",
						"param_source": "chipperfn",
						"service":      service,
						"status":       alog.StatusFail,
						"intent":       intent,
						"error":        err,
						"error_code":   errCode,
						"msg":          "chipperfn fails to parse",
					}.LogC(ctx)
				}
			}

			// Do we have a result?
			if r != nil {
				chipperfnLog := alog.Info{
					"action":  "chipperfn_result",
					"service": service,
					"status":  alog.StatusOK,
					"intent":  intent,
					"msg":     "chipperfn correctly parse entity",
				}
				if logTranscript {
					chipperfnLog["result"] = r
				}
				chipperfnLog.LogC(ctx)
				chippermetrics.CountChipperfnSuccess.Inc(1)
				return &chipperfunction.ChipperFunctionResult{
					Parameters: r.Result,
					NewIntent:  r.IntentName,
				}
			}

		}

	}

	// no result from chipperfn
	alog.Info{
		"action":            "get_parameters",
		"param_source":      "chipper_local",
		"service":           service,
		"intent":            intent,
		"chipperfn_enabled": (chipperfnClient != nil),
	}.LogC(ctx)

	// for weather intent, if chipperfn does not return result
	// (location search fails, deadline-exceed, context-canceled etc.),
	// return intent_weather_unknownlocation
	if intent == "intent_weather_extend" {
		logEntry := alog.Info{
			"action":  "chipperfn_weather_nil",
			"status":  alog.StatusFail,
			"service": service,
		}
		if logTranscript {
			logEntry["parameter"] = parameters
			logEntry["slots"] = slots
		}
		logEntry.LogC(ctx)

		return &chipperfunction.ChipperFunctionResult{
			NewIntent:  chipperfunction.IntentNoLocation,
			Parameters: nil,
		}
	}

	if service == GOOG {
		p := chipperfunction.ParseDialogflow(intent, text, parameters)
		return &p

	} else if service == LEX {
		return &chipperfunction.ChipperFunctionResult{
			Parameters: chipperfunction.ParseLexEntities(intent, slots),
		}
	}
	alog.Warn{
		"action":  "get_parameters",
		"error":   "invalid service type",
		"service": service,
	}.LogC(ctx)
	return nil
}

//
// End - Parameter Parsing
//

//
// Start - Device Id Related
//

var (
	validRobotId = regexp.MustCompile(`^vic:[a-f0-9]{8}$`)
	validDevID   = regexp.MustCompile(`^vic:[a-zA-Z0-9_.+-]+@anki.com:[0-9]+$`)

	ErrorEmptyRequestorId       = status.Errorf(codes.NotFound, "empty token requestor id")
	ErrorInvalidPbDeviceId      = status.Errorf(codes.InvalidArgument, "invalid protobuf device id")
	ErrorInvalidRequestorFormat = status.Errorf(codes.InvalidArgument, "invalid token requestor id format")
)

//
// getDeviceId returns a valid device_id from the token or protobuf "device_id" field.
//
// If there is a valid token, extract a device_id by:
// 1. if token.RequestorId is a valid ESN string, extract 8-char ESN string. e.g. device_id = "00e20123"
// 2. otherwise, RequestorId must match the developer id format. e.g. device_id = "kingshy@anki.com"
// If neither are found, return ErrorInvalidRequestorFormat
//
// 3. In cases where auth is disabled (say, in local testing), check the protobuf field device_id.
// This field cannot be empty and it must have >= 8 chars.
// The final device_id := TestDeviceIdPrefix + "_" + [first 8 chars of protobuf device_id]
//
// Some examples:
// "1234567", "", "123" -> returns ErrorInvalidPbDeviceId
// "123456789" -> returns device_id = "[TestDeviceIdPrefix]_12345678"
// "blach*($^&#$" -> returns device_id = "[TestDeviceIdPrefix]_blach*($"
//
// Note: DAS event RobotId will only contain device_id from case #1. All others will have RobotId = ""
//
func getDeviceId(ctx context.Context, requestDeviceId string) (string, bool, error) {
	token := token_model.GetAccessToken(ctx)

	// if token is present, it has to be a robotId or developerId
	if token != nil {
		return getDeviceIdFromToken(token.RequestorId)
	}

	// no token present, running chipper with no jwt required, only for developer local testing
	alog.Warn{
		"action": "get_device_id",
		"status": alog.StatusFail,
		"msg":    "no jwt token found",
	}.LogC(ctx)

	// get deviceId from the first 8 chars of the protobuf field and add a random prefix
	if requestDeviceId != "" && len(requestDeviceId) >= RobotIdLength {
		deviceId := TestDeviceIdPrefix + "_" + requestDeviceId[:RobotIdLength]
		alog.Info{
			"action": "get_pb_device_id",
			"device": deviceId,
		}.LogC(ctx)
		return deviceId, false, nil
	}
	return EmptyDeviceId, false, ErrorInvalidPbDeviceId
}

// getDeviceIdFromToken will either get robotId or developerId from the JWT token
func getDeviceIdFromToken(s string) (string, bool, error) {
	if s == "" {
		return EmptyDeviceId, false, ErrorEmptyRequestorId
	}

	// check if it's a robot
	if validRobotId.MatchString(s) {
		// if string passes the regex, it will have length=2
		parts := strings.Split(s, ":")
		return parts[1], true, nil
	}

	// check if it's a valid non-robot id
	if validDevID.MatchString(s) {
		parts := strings.Split(s, ":")
		return parts[1], false, nil
	}

	return EmptyDeviceId, false, ErrorInvalidRequestorFormat
}

//
// End - Device Id
//

//
// Start - Helpers (no testing required)
//

func createSessionId(deviceId, session string) string {
	return fmt.Sprintf("%s-%s", deviceId, session)
}

// createSessionHash creates a hash for Dialogflow session and Lex Request Id
// this will prevent special chars in the device/session from creating an invalid string
func createSessionHash(deviceId, session string) string {
	h := sha256.New()
	h.Write([]byte(createSessionId(deviceId, session)))
	return fmt.Sprintf("%x", h.Sum(nil))[:MaxSessionLength]
}

func validateLanguage(language pb.LanguageCode) (string, error) {
	lang, ok := LanguageCodes[language]
	if !ok {
		return "", status.Errorf(codes.InvalidArgument, "invalid lang code %s", language.String())
	}
	return lang, nil
}

func validateAudioEncoding(encoding pb.AudioEncoding) (dialogflow.AudioEncoding, error) {
	codec, ok := AudioCodecs[encoding]
	if !ok {
		return codec, status.Errorf(codes.InvalidArgument, "invalid audio encoding %s", encoding.String())
	}
	return codec, nil
}

func checkRequestError(err error) {
	if grpc.Code(err) != codes.Canceled {
		chippermetrics.CountStreamRequestError.Inc(1)
	}
}

// computeRate is used check audio stream rate for debugging
func computeRate(numBytes int, timeSince time.Duration, audioEncoding pb.AudioEncoding) int64 {
	timeTaken := float64(timeSince.Nanoseconds()) / float64(time.Second)
	switch audioEncoding {
	case pb.AudioEncoding_LINEAR_PCM:
		return int64((float64(numBytes) * 0.5) / timeTaken)
	case pb.AudioEncoding_OGG_OPUS:
		return -1
	}
	return 0
}

func flushAudio(audioChan chan []byte, req chipperRequest, svc string) bytes.Buffer {
	alog.Debug{
		"action":                 "flush_audio_buffer",
		"svc":                    svc,
		"device":                 req.device,
		"session":                req.session,
		"remaining_audio_length": len(audioChan),
	}.Log()

	if len(audioChan) == 0 {
		// empty channel, return an empty buffer
		return *bytes.NewBuffer([]byte{})
	}

	audioBytes := *bytes.NewBuffer(<-audioChan)
	nChan := len(audioChan)
	for i := 0; i < nChan; i++ {
		audioBytes.Write(<-audioChan)
	}

	return audioBytes
}

func hypothesisStoreTimestamp(dt time.Time) string {
	return dt.Format(HypothesisTimeFormat)
}

func durationMs(start time.Time) float32 {
	return float32(time.Since(start)) / float32(time.Millisecond)
}

func logRecvAudioSummary(req chipperRequest, svc string, frames, totalBytes, sentBytes int, elapsed audioElapsedMs) {
	alog.Info{
		"action":           "recv_audio",
		"status":           "summary",
		"svc":              svc,
		"total_frames":     frames,
		"encoding":         req.firstReq.AudioEncoding,
		"total_bytes_recv": totalBytes, // ogg bytes
		"total_bytes_sent": sentBytes,  // pcm bytes
		"time_elapsed_ms":  elapsed,
		"device":           req.device,
		"session":          req.session,
	}.LogC(req.ctx)
}

// logSendClientError outputs the right log-level based on the error code.
// When returning the final results for requests on slower connection,
// we might get deadline exceeded or transport closing errors
// track these with info logs
// log all other errors as warnings
func logSendClientError(req chipperRequest, svc, msg string, err error, isFinalTime *time.Time, intentLatency float32) {

	chippermetrics.CountStreamResponseError.Inc(1)

	errCode := grpc.Code(err)
	logEntry := alog.Info{
		"action":     "send_client",
		"status":     alog.StatusFail,
		"error":      err,
		"error_code": errCode,
		"svc":        svc,
		"device":     req.device,
		"session":    req.session,
		"msg":        msg,
	}

	if isFinalTime != nil {
		// Dialogflow
		logEntry["intent_latency_ms"] = durationMs(*isFinalTime)
		chippermetrics.LatencyDF.UpdateSince(*isFinalTime)
	} else if intentLatency > float32(0) {
		// Lex
		logEntry["intent_latency_ms"] = intentLatency
	}

	if errCode == codes.DeadlineExceeded || errCode == codes.Unavailable {
		logEntry.LogC(req.ctx)
		if errCode == codes.DeadlineExceeded {
			chippermetrics.CountSendClientDeadline.Inc(1)
		} else {
			chippermetrics.CountSendClientUnavailable.Inc(1)
		}

	} else {
		logWarning := alog.Warn{}
		for k, v := range logEntry {
			logWarning[k] = v
		}
		logWarning["status"] = alog.StatusError
		logWarning.LogC(req.ctx)
		chippermetrics.CountSendClientOtherError.Inc(1)
	}
}

// logRecvAudioErrors uses different log level for different recv_audio errors
// There are three scenarios where the client will stop streaming:
// 1. EOF, client successfully do a stream.CloseSend() after getting the result
// 2. context-canceled, client closes the connection after getting the result
// 3. deadline-exceeded, client timed out, usually due to slow initial connections
// log these as info for tracking
// log all other errors as warnings
func logRecvAudioErrors(req chipperRequest, svc string, err error) {
	errCode := grpc.Code(err)
	logEntry := alog.Info{
		"action":           "recv_audio",
		"svc":              svc,
		"error":            err,
		"error_code":       errCode,
		"device":           req.device,
		"session":          req.session,
		"send_duration_ms": durationMs(req.reqTime),
	}

	if err == io.EOF || errCode == codes.Canceled || errCode == codes.DeadlineExceeded {
		logEntry["status"] = "client_stop_streaming"
		logEntry["msg"] = "client stop sending, close stream and exit"
		logEntry.LogC(req.ctx)

		if errCode == codes.Canceled {
			chippermetrics.CountRecvAudioCanceled.Inc(1)
		} else if errCode == codes.DeadlineExceeded {
			chippermetrics.CountRecvAudioDeadline.Inc(1)
		}

	} else {

		logWarning := alog.Warn{
			"status": "unexpected_error",
			"msg":    "unexpected error, close stream and exit",
		}
		for k, v := range logEntry {
			logWarning[k] = v
		}
		logWarning.LogC(req.ctx)
		chippermetrics.CountRecvAudioOtherError.Inc(1)
	}
}

// End - Helpers
