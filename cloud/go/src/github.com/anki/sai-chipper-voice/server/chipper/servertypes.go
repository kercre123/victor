package chipper

import (
	"bytes"
	"context"
	"time"

	"github.com/anki/houndify-sdk-go/houndify"

	"github.com/anki/sai-chipper-voice/audiostore"
	"github.com/anki/sai-chipper-voice/client/chipperfn"
	"github.com/anki/sai-chipper-voice/conversation/dialogflow"
	"github.com/anki/sai-chipper-voice/conversation/microsoft"
	"github.com/anki/sai-chipper-voice/hstore"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/requestrouter"
)

type Server struct {
	logTranscript          bool
	dialogflowProject      DialogflowConfig
	dialogflowGamesProject DialogflowConfig
	microsoftConfig        MicrosoftConfig
	luisClient             *microsoft.LuisClient
	lexClient              *LexClient
	houndifyClient         *houndify.Client
	intentSvcEnabled       map[pb.IntentService]bool

	audioStoreClient *audiostore.Client
	saveAudio        bool
	dasClient        *DasClient
	chipperfnClient  chipperfn.Client
	hypostoreClient  *hstore.Client
	requestRouter    *requestrouter.RequestRouter
}

// vcRequest holds the parameters related to a chipper request (voice or text)
type chipperRequest struct {
	stream          pb.ChipperGrpc_StreamingIntentServer
	firstReq        *pb.StreamingIntentRequest
	device          string
	deviceIsEsn     bool
	session         string
	langString      string
	audioCodec      dialogflow.AudioEncoding
	reqTime         time.Time
	asr             bool
	mode            pb.RobotMode
	langCode        pb.LanguageCode
	ctx             context.Context
	version         *requestrouter.FirmwareIntentVersion
	dasRobotVersion string
}

type chipperResult struct {
	intent      *pb.IntentResponse
	kg          *pb.KnowledgeGraphResponse
	kgResponse  *houndify.HoundifyResponse
	rawParams   string
	audio       *bytes.Buffer
	err         error
	audioLength *time.Duration
}

type audioResponse struct {
	intentResponse *pb.IntentResponse
	audioLength    *time.Duration
	audioBytes     *bytes.Buffer
}

type kgRequest struct {
	stream          pb.ChipperGrpc_StreamingKnowledgeGraphServer
	firstReq        *pb.StreamingKnowledgeGraphRequest
	device          string
	session         string
	deviceIsEsn     bool
	langString      string
	audioCodec      dialogflow.AudioEncoding
	reqTime         time.Time
	fw              string
	dasRobotVersion string
	timezone        string
}

type connectionRequest struct {
	stream          pb.ChipperGrpc_StreamingConnectionCheckServer
	firstReq        *pb.StreamingConnectionCheckRequest
	device          string
	session         string
	deviceIsEsn     bool
	reqTime         time.Time
	requestId       string
	totalAudioMs    uint32
	audioPerRequest uint32
}
