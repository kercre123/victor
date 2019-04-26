package chipperstub

import (
	"io"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-go-util/log"
)

type mockChipperServerStream struct {
	allowCloseStream bool
	disconnectStream bool
	eof              int32
	req              pb.StreamingIntentRequest
	grpc.ServerStream
}

func NewMockChipperServerStream(req pb.StreamingIntentRequest, closeStream, disconnect bool) *mockChipperServerStream {
	return &mockChipperServerStream{
		allowCloseStream: closeStream,
		disconnectStream: disconnect,
		req:              req,
		eof:              int32(0),
	}
}

func (s *mockChipperServerStream) setEOF() {
	atomic.StoreInt32(&s.eof, int32(1))
}

func (s *mockChipperServerStream) isEOF() bool {
	if atomic.LoadInt32(&s.eof) != 0 {
		return true
	}
	return false
}

// Send mocks server sending responses to client (robot)
func (s *mockChipperServerStream) Send(resp *pb.IntentResponse) error {
	time.Sleep(2 * time.Millisecond)
	alog.Info{"action": "chipperstub_send", "resp": resp}.Log()
	if s.allowCloseStream && resp != nil && resp.IsFinal == true {
		alog.Info{"action": "sets_EOF"}.Log()
		s.setEOF()
	}
	return nil
}

// Recv mocks server receiving request from client
func (s *mockChipperServerStream) Recv() (*pb.StreamingIntentRequest, error) {
	time.Sleep(1 * time.Millisecond)
	if s.isEOF() {
		alog.Info{"action": "chipperstub_gets_EOF"}.Log()
		return nil, io.EOF
	}
	if s.disconnectStream {
		// pretend that we have sent one second of audio
		time.Sleep(time.Second)
		return nil, status.Error(codes.Canceled, "context canceled")
	}

	return &s.req, nil
}
