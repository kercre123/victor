package dialogflow

import (
	"context"
	"sync/atomic"
	"time"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	df "github.com/anki/sai-chipper-voice/conversation/dialogflow"
	"github.com/anki/sai-go-util/log"
)

type mockStreamingClient struct {
	maxSend        uint32
	countRecv      uint32
	countSend      uint32
	isFinal        bool
	sentResult     bool
	expectedResult *df.IntentResponse
	expectedError  error
	deadline       time.Time
}

func NewMockStreamingClient(ctx context.Context, maxSend uint32, expected *df.IntentResponse, err error) *mockStreamingClient {
	deadline, ok := ctx.Deadline()
	if !ok {
		deadline = time.Now().Add(time.Second * 5)
	}
	return &mockStreamingClient{
		maxSend:        maxSend,
		expectedResult: expected,
		expectedError:  err,
		isFinal:        false,
		sentResult:     false,
		deadline:       deadline,
	}
}

func (s *mockStreamingClient) Close() error {
	return nil
}

func (s *mockStreamingClient) CloseSend() error {
	return nil
}

func (s *mockStreamingClient) Send(audio []byte) error {
	atomic.AddUint32(&s.countSend, 1)
	alog.Debug{
		"action": "stub_send",
		"size":   len(audio),
		"packet": atomic.LoadUint32(&s.countSend),
	}.Log()

	time.Sleep(100 * time.Millisecond)
	return nil
}

func (s *mockStreamingClient) Recv() (bool, *df.IntentResponse, error) {
	alog.Info{"action": "mock_dialogflow_recv", "is_final": s.isFinal}.Log()
	atomic.AddUint32(&s.countRecv, 1)
	sent := atomic.LoadUint32(&s.countSend)

	time.Sleep(100 * time.Millisecond)
	if time.Now().After(s.deadline) {
		return false, nil, status.Error(codes.DeadlineExceeded, "deadline exceeded")
	}

	if s.sentResult {
		alog.Info{"action": "result_sent"}.Log()
		return true, nil, nil
	}

	if s.isFinal && s.expectedResult.ResponseType == df.ResponseTypeIntent {
		s.sentResult = true
		return true, s.expectedResult, nil
	}

	if s.maxSend > 0 && sent > (s.maxSend-1) {
		alog.Info{
			"action":      "stub_recv_greater",
			"send_count":  sent,
			"recv_count":  atomic.LoadUint32(&s.countRecv),
			"isFinal":     s.isFinal,
			"sent_result": s.sentResult,
		}.Log()
		if s.expectedError != nil {
			return false, nil, s.expectedError
		}

		if !s.isFinal {
			s.isFinal = true
		}
		resp := &df.IntentResponse{
			ResponseType:     df.ResponseTypeFinalSpeech,
			QueryText:        s.expectedResult.QueryText,
			SpeechConfidence: s.expectedResult.SpeechConfidence,
			IsFinal:          true,
		}
		return true, resp, nil
	}
	return false, nil, nil
}
