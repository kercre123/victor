package chipper

import (
	"context"
	"encoding/json"
	"strings"
	"sync/atomic"
	"time"

	"github.com/nu7hatch/gouuid"

	"github.com/anki/sai-das-client/das"
	"github.com/anki/sai-das-client/events"
	"github.com/anki/sai-go-util/log"
)

const (
	intentMatchEvent    = "intent.match"
	intentErrorEvent    = "intent.error"
	intentTimeoutEvent  = "intent.timeout"
	knowledgeGraphEvent = "knowledgegraph.result"
	confidenceScaling   = float32(1000)
	RequestTypeAudio    = 1
	RequestTypeText     = 2
)

type DasClient struct {
	c       *das.BufferedClient
	product string
	src     string
	seqId   string
	seq     int64
}

func NewDasClient(product, source, url string) (*DasClient, error) {
	dasConfig := das.NewConfig(product, url)
	dasClient, err := das.NewBufferedClient(dasConfig)
	if err != nil {
		return nil, err
	}

	seqId, _ := uuid.NewV4()

	return &DasClient{
		c:       dasClient,
		product: product,
		src:     source,
		seqId:   seqId.String(),
		seq:     int64(0),
	}, nil
}

func (d *DasClient) Stop() {
	d.c.Stop()
}

func (d *DasClient) CreateEvent(result *chipperResult, req chipperRequest, reqType int64, bootId string) events.Event {
	seq := atomic.LoadInt64(&d.seq)
	atomic.AddInt64(&d.seq, int64(1))

	eventName := intentMatchEvent
	if result.intent.IntentResult.Action == NoAudioIntent {
		eventName = intentTimeoutEvent
	}

	robotId := ""
	if req.deviceIsEsn {
		robotId = strings.ToLower(req.device)
	}

	return &events.VictorCloudEvent{
		Source:       d.src,
		SequenceId:   d.seqId,
		Seq:          seq,
		Event:        eventName,
		Ts:           req.reqTime.UTC().UnixNano() / 1e6,
		Level:        das.LevelEvent,
		RobotId:      robotId,
		RobotVersion: req.dasRobotVersion,
		BootId:       strings.TrimSpace(bootId),
		S1:           result.intent.IntentResult.Action,
		S2:           req.langString,
		S3:           req.session,
		I1:           reqType,
		I2:           int64(result.intent.IntentResult.Service), // 1: goog, 2: ms, 3: lex
		I3:           getAudioLengthMs(result.audioLength),
	}
}

func (d *DasClient) CreateKnowledgeGraphEvent(result *chipperResult, req kgRequest, bootId string) events.Event {
	seq := atomic.LoadInt64(&d.seq)
	atomic.AddInt64(&d.seq, int64(1))

	robotId := ""
	if req.deviceIsEsn {
		robotId = strings.ToLower(req.device)
	}

	return &events.VictorCloudEvent{
		Source:       d.src,
		SequenceId:   d.seqId,
		Seq:          seq,
		Event:        knowledgeGraphEvent,
		Ts:           req.reqTime.UTC().UnixNano() / 1e6,
		Level:        das.LevelEvent,
		RobotId:      robotId,
		RobotVersion: req.dasRobotVersion,
		BootId:       strings.TrimSpace(bootId),
		S1:           result.kg.CommandType,
		S2:           req.langString,
		S3:           req.session,
		I3:           getAudioLengthMs(result.audioLength),
	}
}

func (d *DasClient) Send(ctx context.Context, e events.Event) {
	d.c.SendEvent(ctx, e) // buffered send, no error or msgId returned
	e2, _ := json.Marshal(e)
	alog.Debug{"action": "send_das", "event": e2}.Log()
}

func transformConf(s float32) int64 {
	// transform confidence into an integer
	return int64(s * confidenceScaling)
}

func getAudioLengthMs(audioLength *time.Duration) int64 {
	audioLengthMs := int64(-1)
	if audioLength != nil {
		audioLengthMs = int64(*audioLength / time.Millisecond) // audio length in ms
	}
	return audioLengthMs
}
