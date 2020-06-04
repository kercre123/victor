package chipper

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/golang/protobuf/ptypes/struct"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"github.com/anki/sai-go-util/log"

	df "github.com/anki/sai-chipper-voice/conversation/dialogflow"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	dfpb "google.golang.org/genproto/googleapis/cloud/dialogflow/v2"

	"github.com/anki/sai-chipper-voice/requestrouter"
	"github.com/anki/sai-chipper-voice/teststubsmocks/chipperstub"
	"github.com/anki/sai-chipper-voice/teststubsmocks/dialogflowmock"
	dfstub "github.com/anki/sai-chipper-voice/teststubsmocks/dialogflowstub"
)

const TestTimeOut = 5 * time.Second

var PermissionDeniedError error = status.Error(
	codes.PermissionDenied,
	"IAM permission 'dialogflow.sessions.streamingDetectIntent'"+
		"on 'projects/victor-dev-en-us/agent' denied")

func NewTestServer() *Server {
	projName := "fake-df"
	projCreds := "{\"type\": \"service_account\", \"project_id\": \"fake-df\", \"client_email\": \"test@test.com\"}"

	return &Server{
		saveAudio:     false,
		logTranscript: false,
		intentSvcEnabled: map[pb.IntentService]bool{
			pb.IntentService_DIALOGFLOW: true,
			pb.IntentService_BING_LUIS:  true,
			pb.IntentService_LEX:        true,
		},
		dialogflowProject: DialogflowConfig{
			ProjectName: &projName,
			Creds:       &projCreds,
			CredEnvVar:  "TEST",
		},
	}
}

//
// TextIntent Tests
//

func TestDialogflowTextIntent(t *testing.T) {

	// create text-request data
	version := requestrouter.CreateVersion(
		"dialogflow", "en-us", "vc", requestrouter.DefaultFWString,
		"fake-df", "", "TEST", "", "", true,
	)
	ctx := context.Background()
	ir := chipperRequest{
		device:     "device",
		session:    "session",
		langString: "en-US",
		reqTime:    time.Now(),
		audioCodec: df.UnSpecifiedEncoding,
		mode:       pb.RobotMode_VOICE_COMMAND,
		ctx:        ctx,
		version:    version,
	}

	query := "My name is LeBron James"

	// create parameters to return
	username := map[string]string{"username": "LeBron James"}
	names := make([]*structpb.Value, 2)
	names[0] = &structpb.Value{
		Kind: &structpb.Value_StringValue{
			StringValue: "Lebron",
		},
	}
	names[1] = &structpb.Value{
		Kind: &structpb.Value_StringValue{
			StringValue: "James",
		},
	}

	params := &structpb.Struct{
		Fields: map[string]*structpb.Value{
			"given_name": {
				Kind: &structpb.Value_ListValue{
					ListValue: &structpb.ListValue{
						Values: names,
					},
				},
			},
		},
	}

	// create dialogflow response
	resp := &df.IntentResponse{
		Session:              ir.session,
		QueryType:            df.QueryTypeText,
		IsFinal:              true,
		QueryText:            query,
		Action:               "intent_names_username_extend",
		Parameters:           params,
		AllParametersPresent: true,
	}

	// mock dialogflow
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	mockDf := dialogflowmock.NewMockSessionClient(ctrl)

	mockDf.EXPECT().DetectTextIntent(gomock.Eq(ctx), query, ir.langString).Return(resp, nil)

	// get text intent
	s := NewTestServer()
	result := s.DialogflowTextIntent(mockDf, query, ir)

	require.NoError(t, result.err)

	// check that dialogflow result is parsed properly
	assert.Equal(t, ir.device, result.intent.DeviceId)
	assert.Equal(t, resp.Action, result.intent.IntentResult.Action)
	assert.Equal(t, query, result.intent.IntentResult.QueryText)
	assert.NotEqual(t, "", result.intent.Session)
	assert.Equal(t, username["username"], result.intent.IntentResult.Parameters["username"])
}

func TestDialogflowTextIntentPermissionDenied(t *testing.T) {
	// create text-request data
	ctx := context.Background()
	version := requestrouter.CreateVersion(
		"dialogflow", "en-us", "vc", requestrouter.DefaultFWString,
		"fake-df", "", "TEST", "", "", true,
	)
	ir := chipperRequest{
		device:     "device",
		session:    "session",
		langString: "en-US",
		reqTime:    time.Now(),
		audioCodec: df.UnSpecifiedEncoding,
		mode:       pb.RobotMode_VOICE_COMMAND,
		ctx:        ctx,
		version:    version,
	}

	// mock dialogflow result
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()
	mockDf := dialogflowmock.NewMockSessionClient(ctrl)

	mockDf.EXPECT().DetectTextIntent(gomock.Eq(ctx), gomock.Any(), gomock.Any()).Return(nil, PermissionDeniedError)

	// get text intent
	s := NewTestServer()
	query := "My name is LeBron James"
	result := s.DialogflowTextIntent(mockDf, query, ir)
	assert.Nil(t, result.intent)
	assert.NotNil(t, result.err)
	assert.Equal(t, codes.PermissionDenied.String(), status.Code(result.err).String())
}

//
// StreamingIntent Tests
// The main func we're testing is dialogflowStreamingIntent()
//

func mockFirstRequest(session string) *pb.StreamingIntentRequest {
	return &pb.StreamingIntentRequest{
		Session:         session,
		DeviceId:        "test",
		InputAudio:      []byte{byte(123)},
		LanguageCode:    pb.LanguageCode_ENGLISH_US,
		SpeechOnly:      false,
		Mode:            pb.RobotMode_VOICE_COMMAND,
		AudioEncoding:   pb.AudioEncoding_LINEAR_PCM,
		SingleUtterance: true,
		IntentService:   pb.IntentService_DIALOGFLOW,
		SaveAudio:       false,
	}
}

var mockResponse = &df.IntentResponse{
	Session:              "test",
	QueryType:            df.QueryTypeStreaming,
	ResponseType:         df.ResponseTypeIntent,
	IsFinal:              true,
	QueryText:            "good to see you",
	Action:               "intent_greeting_hello",
	Parameters:           nil,
	AllParametersPresent: true,
}

var mockTimeoutResponse = &df.IntentResponse{
	Session:          "test",
	QueryType:        df.QueryTypeStreaming,
	ResponseType:     df.ResponseTypeFinalSpeech,
	IsFinal:          true,
	QueryText:        "good to see",
	SpeechConfidence: float32(0.5),
}

func mockRequest(
	stubStream pb.ChipperGrpc_StreamingIntentServer,
	firstReq *pb.StreamingIntentRequest,
	ctx context.Context) *chipperRequest {
	version := requestrouter.CreateVersion(
		"dialogflow", "en-us", "vc", requestrouter.DefaultFWString,
		"fake-df", "", "TEST", "", "", true,
	)
	return &chipperRequest{
		stream:     stubStream,
		firstReq:   firstReq,
		device:     "test",
		session:    firstReq.Session,
		langString: "en-US",
		audioCodec: dfpb.AudioEncoding_AUDIO_ENCODING_LINEAR_16,
		reqTime:    time.Now(),
		asr:        false,
		mode:       firstReq.Mode,
		langCode:   firstReq.LanguageCode,
		ctx:        ctx,
		version:    version,
	}
}

func TestDialogflowStreamingIntent(t *testing.T) {
	alog.SetLevelByString("ERROR")

	// fake dialogflow client
	ctx := context.Background()
	dataPacketsToSend := uint32(10)
	stubClient := dfstub.NewMockStreamingClient(ctx, dataPacketsToSend, mockResponse, nil)

	// fake chipper server stream
	allowStreamToClose := true // simulate client half-closing stream after getting final intent-result
	disconnect := false
	firstReq := mockFirstRequest("test-success")
	stubStream := chipperstub.NewMockChipperServerStream(*firstReq, allowStreamToClose, disconnect)

	// create new test server
	s := NewTestServer()

	// send request to fake dialogflow
	req := mockRequest(stubStream, firstReq, ctx)

	result := s.dialogflowStreamingIntent(stubClient, *req)
	require.NoError(t, result.err)
	assert.Equal(t, "intent_greeting_hello", result.intent.IntentResult.Action)
}

func TestDialogflowStreamingIntentTimeout(t *testing.T) {
	alog.SetLevelByString("ERROR")

	checkTime := time.Now().Add(TestTimeOut)
	ctx, _ := context.WithTimeout(context.Background(), TestTimeOut)

	// fake dialogflow client to send some audio
	dataPacketsToSend := uint32(10)
	stubClient := dfstub.NewMockStreamingClient(ctx, dataPacketsToSend, mockTimeoutResponse, nil)

	// fake chipper server stream
	allowStreamToClose := true // simulate client half-closing stream after getting final intent-result
	disconnect := false
	firstReq := mockFirstRequest("test-timeout")
	stubStream := chipperstub.NewMockChipperServerStream(*firstReq, allowStreamToClose, disconnect)

	// create new test server
	s := NewTestServer()

	// fake chipper request
	req := mockRequest(stubStream, firstReq, ctx)
	result := s.dialogflowStreamingIntent(stubClient, *req)

	require.NoError(t, result.err)
	assert.Equal(t, TimeoutIntent, result.intent.IntentResult.Action)
	assert.Equal(t, mockTimeoutResponse.QueryText, result.intent.IntentResult.QueryText)

	passedTime := time.Now().After(checkTime)
	assert.Equal(t, true, passedTime)
}

func TestDialogflowStreamingIntentNoAudio(t *testing.T) {
	alog.SetLevelByString("ERROR")

	checkTime := time.Now().Add(TestTimeOut)
	ctx, _ := context.WithTimeout(context.Background(), TestTimeOut)

	// fake dialogflow client to send lots of packets
	dataPacketsToSend := uint32(1100)
	stubClient := dfstub.NewMockStreamingClient(ctx, dataPacketsToSend, nil, nil)

	// fake chipper server stream
	allowStreamToClose := true // simulate client half-closing stream after getting final intent-result
	disconnect := false
	firstReq := mockFirstRequest("test-noaudio")
	stubStream := chipperstub.NewMockChipperServerStream(*firstReq, allowStreamToClose, disconnect)

	// create new test server
	s := NewTestServer()

	// fake chipper request
	req := mockRequest(stubStream, firstReq, ctx)
	result := s.dialogflowStreamingIntent(stubClient, *req)

	require.NoError(t, result.err)
	assert.Equal(t, NoAudioIntent, result.intent.IntentResult.Action)

	passedTime := time.Now().After(checkTime)
	assert.Equal(t, true, passedTime)
}

func TestDialogflowStreamClientDisconnect(t *testing.T) {
	alog.SetLevelByString("ERROR")
	dataPacketsToSend := uint32(10)
	ctx, _ := context.WithTimeout(context.Background(), TestTimeOut)
	stubClient := dfstub.NewMockStreamingClient(ctx, dataPacketsToSend, mockResponse, nil)

	// fake chipper server stream
	allowStreamToClose := true // simulate client half-closing stream after getting final intent-result
	disconnect := true         // simulate client disconnecting
	firstReq := mockFirstRequest("test-disconnect")
	stubStream := chipperstub.NewMockChipperServerStream(*firstReq, allowStreamToClose, disconnect)

	// create new test server
	s := NewTestServer()
	req := mockRequest(stubStream, firstReq, ctx)

	result := s.dialogflowStreamingIntent(stubClient, *req)
	require.NoError(t, result.err)

	// since dialogflow Recv is blocking, even when the client disconnect,
	// we will wait for dialogflow to timeout and return a no_audio intent
	assert.Equal(t, NoAudioIntent, result.intent.IntentResult.Action)

}

func TestDialogflowStreamingIntentPermissionDenied(t *testing.T) {
	alog.SetLevelByString("ERROR")

	// fake dialogflow client
	dataPacketsToSend := uint32(3)
	ctx, _ := context.WithTimeout(context.Background(), TestTimeOut)
	stubClient := dfstub.NewMockStreamingClient(ctx, dataPacketsToSend, nil, PermissionDeniedError)

	// fake chipper server stream
	allowStreamToClose := true // simulate client half-closing stream after getting final intent-result
	disconnect := false
	firstReq := mockFirstRequest("test-permission-denied")
	stubStream := chipperstub.NewMockChipperServerStream(*firstReq, allowStreamToClose, disconnect)

	// create new test server
	s := NewTestServer()

	req := mockRequest(stubStream, firstReq, ctx)
	result := s.dialogflowStreamingIntent(stubClient, *req)
	require.NotNil(t, result.err)
	require.Nil(t, result.intent)
	assert.Equal(t, codes.PermissionDenied.String(), status.Code(result.err).String())

}

// This test is not finishing because chipper is not handling the ase where the client
// is not closing the stream after getting results.
//func TestDialogflowStreamClientNotClose(t *testing.T) {
//
//	dataPacketsToSend := uint32(10) // set maxSend in mocked client to 0
//	stubClient := df.NewMockStreamingClient(context.Background(), dataPacketsToSend, mockResponse, nil)
//
//	// fake chipper server stream
//	allowStreamToClose := false // simulate client half-closing stream after getting final intent-result
//	stubStream := mock_chipperpb.NewMockChipperServerStream(firstReq, allowStreamToClose)
//
//	req := mockRequest(stubStream)
//
//	// create new test server
//	s := NewTestServer()
//	result, _, err := s.dialogflowStreamingIntent(stubClient, *req)
//	require.NoError(t, err)
//	assert.Equal(t, "intent_greeting_hello", result.IntentResult.Action)
//}
