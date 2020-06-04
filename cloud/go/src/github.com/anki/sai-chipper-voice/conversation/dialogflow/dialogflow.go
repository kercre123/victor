// Copyright 2017 Anki, Inc.

// Package dialogflow provides a wrapper for communicating with the Google
// Dialogflow Enterprise Edition
package dialogflow

import (
	"context"
	"fmt"
	"os"

	dialogflow_v2 "cloud.google.com/go/dialogflow/apiv2"
	google_protobuf4 "github.com/golang/protobuf/ptypes/struct"
	"google.golang.org/api/option"
	dialogflowpb "google.golang.org/genproto/googleapis/cloud/dialogflow/v2"

	"github.com/anki/sai-service-framework/grpc/grpcclient"
)

type QueryType int32

type ResponseType int32

const (
	QueryTypeText      QueryType = 1
	QueryTypeAudio     QueryType = 2
	QueryTypeStreaming QueryType = 3

	// ResponseTypePartialSpeech indicates IntentResponse only has the partial speech-result
	ResponseTypePartialSpeech ResponseType = 4
	// ResponseTypeFinalSpeech indicates IntentResponse has the final speech-result
	ResponseTypeFinalSpeech ResponseType = 5
	// ResponseTypeIntent indicates IntentResponse has the final intent-result
	ResponseTypeIntent ResponseType = 6
)

var QueryTypeName = map[QueryType]string{
	1: "TEXT",
	2: "AUDIO",
	3: "STREAMING",
}

// IntentResponse - intent result
type IntentResponse struct {
	Session              string                   `json:"-"`
	QueryType            QueryType                `json:"-"`
	ResponseType         ResponseType             `json:"response_type"`
	IsFinal              bool                     `json:"is_final"`
	QueryText            string                   `json:"query_text"`
	Action               string                   `json:"action"`
	IntentConfidence     float32                  `json:"intent_confidence"`
	SpeechConfidence     float32                  `json:"speech_confidence"`
	Parameters           *google_protobuf4.Struct `json:"parameters"`
	AllParametersPresent bool                     `json:"all_parameters_present"`
}

type IntentConfig struct {
	SingleUtterance bool
	Language        string
	AudioEncoding   dialogflowpb.AudioEncoding
}

func (r IntentResponse) String() string {
	return fmt.Sprintf(
		"session: %s\ntype: %s\nquery: %s\nconf: %v\nspeech: %v\nintent: %s",
		r.Session,
		QueryTypeName[r.QueryType],
		r.QueryText,
		r.IntentConfidence,
		r.SpeechConfidence,
		r.Action)
}

type AudioEncoding = dialogflowpb.AudioEncoding

// Dialogflow defaults and parameters

const (
	DefaultSampleRate    = 16000
	DefaultAudioEncoding = dialogflowpb.AudioEncoding_AUDIO_ENCODING_LINEAR_16
	LinearEncoding       = dialogflowpb.AudioEncoding_AUDIO_ENCODING_LINEAR_16
	OggEncoding          = dialogflowpb.AudioEncoding_AUDIO_ENCODING_OGG_OPUS
	UnSpecifiedEncoding  = dialogflowpb.AudioEncoding_AUDIO_ENCODING_UNSPECIFIED
)

type StreamingClient interface {
	Send(audio []byte) error
	Recv() (bool, *IntentResponse, error)
	Close() error
	CloseSend() error
}

// StreamingClient for streaming intent
type streamingClient struct {
	sessionClient *dialogflow_v2.SessionsClient
	stream        dialogflowpb.Sessions_StreamingDetectIntentClient
	session       string
}

func (sc *streamingClient) GetSession() string {
	return sc.session
}

// CreateSessionName in the correct format
func CreateSessionName(projectName, session string) string {
	return fmt.Sprintf("projects/%s/agent/sessions/%s", projectName, session)
}

func CreateSessionNameWithVersion(projectName, version, session string) string {
	return fmt.Sprintf(
		"projects/%s/agent/environments/%s/users/-/sessions/%s",
		projectName, version, session)
}

func getOptions(credEnv string) []option.ClientOption {
	credentialFile := os.Getenv(credEnv)
	return []option.ClientOption{
		option.WithCredentialsFile(credentialFile),
		option.WithGRPCDialOption(grpcclient.WithStandardStreamClientChain()),
		option.WithGRPCDialOption(grpcclient.WithStandardUnaryClientChain()),
	}
}

// NewStreamingClient method to get a StreamClient
func NewStreamingClient(ctx context.Context, projectName, version, session, credEnv string, config IntentConfig) (*streamingClient, error) {

	options := getOptions(credEnv)
	client, err := dialogflow_v2.NewSessionsClient(ctx, options...)
	if err != nil {
		return nil, err
	}
	stream, err := client.StreamingDetectIntent(ctx)
	if err != nil {
		return nil, err
	}

	// send first configuration message
	sessionName := ""
	if version != "" {
		sessionName = CreateSessionNameWithVersion(projectName, version, session)
	} else {
		sessionName = CreateSessionName(projectName, session)
	}

	if err := stream.Send(&dialogflowpb.StreamingDetectIntentRequest{
		Session:         sessionName,
		SingleUtterance: config.SingleUtterance,
		QueryInput: &dialogflowpb.QueryInput{
			Input: &dialogflowpb.QueryInput_AudioConfig{
				AudioConfig: &dialogflowpb.InputAudioConfig{
					AudioEncoding:   config.AudioEncoding,
					SampleRateHertz: DefaultSampleRate,
					LanguageCode:    config.Language,
				},
			},
		},
	}); err != nil {
		return nil, err
	}

	return &streamingClient{
		session:       sessionName,
		stream:        stream,
		sessionClient: client,
	}, nil
}

// CloseSend will half-close the stream
func (s *streamingClient) CloseSend() error {
	return s.stream.CloseSend()
}

// Close will shut down entirely
func (s *streamingClient) Close() error {
	err := s.CloseSend()
	if err != nil {
		return err
	}
	return s.sessionClient.Close()
}

// Send streaming audio
func (s *streamingClient) Send(audio []byte) error {
	if err := s.stream.Send(&dialogflowpb.StreamingDetectIntentRequest{
		InputAudio: audio,
	}); err != nil {
		return err
	}
	return nil
}

// Recv method to receive streaming intent responses
// returns isFinal (bool), intent-result (IntentResponse), error
func (s *streamingClient) Recv() (bool, *IntentResponse, error) {
	response, err := s.stream.Recv()

	if err != nil {
		// alog.Warn{"action": "dialogflow_recv", "status": alog.StatusError, "error": err}.Log()
		return false, nil, err
	}

	if result := response.GetQueryResult(); result != nil {
		detectResponse := IntentResponse{
			Session:              s.session,
			QueryType:            QueryTypeStreaming,
			ResponseType:         ResponseTypeIntent,
			IsFinal:              true,
			QueryText:            result.GetQueryText(),
			Action:               result.Action,
			IntentConfidence:     result.IntentDetectionConfidence,
			SpeechConfidence:     result.SpeechRecognitionConfidence,
			Parameters:           result.GetParameters(),
			AllParametersPresent: result.GetAllRequiredParamsPresent(),
		}
		return true, &detectResponse, nil
	}

	if speechResult := response.GetRecognitionResult(); speechResult != nil {
		// return partial transcript
		speechResponse := IntentResponse{
			Session:          s.session,
			QueryType:        QueryTypeStreaming,
			QueryText:        speechResult.Transcript,
			SpeechConfidence: speechResult.Confidence,
		}

		if speechResult.IsFinal {
			speechResponse.IsFinal = true
			speechResponse.ResponseType = ResponseTypeFinalSpeech
			return true, &speechResponse, nil
		} else {
			speechResponse.IsFinal = false
			speechResponse.ResponseType = ResponseTypePartialSpeech
			return false, &speechResponse, nil
		}
	}

	return false, nil, nil
}

type SessionClient interface {
	DetectTextIntent(ctx context.Context, text, languageCode string) (*IntentResponse, error)
}

type sessionClient struct {
	Client  *dialogflow_v2.SessionsClient
	Session string
}

func NewSessionClient(ctx context.Context, projectName, version, session, credEnv string) (*sessionClient, error) {

	options := getOptions(credEnv)
	client, err := dialogflow_v2.NewSessionsClient(ctx, options...)
	if err != nil {
		return nil, err
	}

	sessionName := ""
	if version != "" {
		sessionName = CreateSessionNameWithVersion(projectName, version, session)
	} else {
		sessionName = CreateSessionName(projectName, session)
	}

	return &sessionClient{
		Client:  client,
		Session: sessionName,
	}, nil
}

func (sc *sessionClient) DetectTextIntent(ctx context.Context, text, languageCode string) (*IntentResponse, error) {
	defer sc.Client.Close()

	queryInput := dialogflowpb.QueryInput{
		Input: &dialogflowpb.QueryInput_Text{
			Text: &dialogflowpb.TextInput{
				Text:         text,
				LanguageCode: languageCode,
			},
		},
	}

	response, err := sc.Client.DetectIntent(ctx, &dialogflowpb.DetectIntentRequest{
		Session:    sc.Session,
		QueryInput: &queryInput,
	})

	if err != nil {
		return nil, err

	}

	result := response.GetQueryResult()
	detectResponse := IntentResponse{
		Session:              sc.Session,
		QueryType:            QueryTypeText,
		IsFinal:              true,
		QueryText:            result.GetQueryText(),
		Action:               result.Action,
		IntentConfidence:     result.IntentDetectionConfidence,
		SpeechConfidence:     0.0,
		Parameters:           result.GetParameters(),
		AllParametersPresent: result.GetAllRequiredParamsPresent(),
	}
	return &detectResponse, nil
}

// DetectAudioIntent process entire chunk of audio data
func DetectAudioIntent(ctx context.Context, projectName, session string, audio []byte, config IntentConfig) (*IntentResponse, error) {

	c, err := dialogflow_v2.NewSessionsClient(ctx)
	if err != nil {
		return nil, err
	}
	defer c.Close()

	audioConfig := &dialogflowpb.InputAudioConfig{
		AudioEncoding:   config.AudioEncoding,
		SampleRateHertz: DefaultSampleRate,
		LanguageCode:    config.Language,
	}

	sessionName := CreateSessionName(projectName, session)
	audioRequest := &dialogflowpb.DetectIntentRequest{
		Session: sessionName,
		QueryInput: &dialogflowpb.QueryInput{
			Input: &dialogflowpb.QueryInput_AudioConfig{
				AudioConfig: audioConfig,
			},
		},
		InputAudio: audio,
	}

	response, err := c.DetectIntent(ctx, audioRequest)
	if err != nil {
		return nil, err
	}

	result := response.GetQueryResult()
	detectResponse := IntentResponse{
		Session:              sessionName,
		QueryType:            QueryTypeAudio,
		IsFinal:              true,
		QueryText:            result.GetQueryText(),
		Action:               result.Action,
		IntentConfidence:     result.IntentDetectionConfidence,
		SpeechConfidence:     result.SpeechRecognitionConfidence,
		Parameters:           result.GetParameters(),
		AllParametersPresent: result.GetAllRequiredParamsPresent(),
	}
	return &detectResponse, nil
}
