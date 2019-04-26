package chipper

import (
	"context"
	"time"

	"github.com/anki/sai-go-util/log"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-chipper-voice/server/chippermetrics"
)

const (
	connectionCheckTimeout = 8 * time.Second
	CHECK                  = "check"
)

func (s *Server) processRequestConnectionCheck(cr connectionRequest) error {
	ctx, cancel := context.WithTimeout(cr.stream.Context(), connectionCheckTimeout)
	defer cancel()

	framesPerRequest := cr.totalAudioMs / cr.audioPerRequest

	var toSend pb.ConnectionCheckResponse
	var err error
	// count frames, we already pulled the first one
	frames := uint32(1)
	toSend.FramesReceived = frames
receiveLoop:
	for {
		select {
		case <-ctx.Done():
			alog.Info{
				"action":             "connection_check_expired",
				"svc":                CHECK,
				"status":             "server_timeout",
				"frames_received":    frames,
				"frames_per_request": framesPerRequest,
				"device":             cr.device,
				"session":            cr.session,
			}.LogC(cr.stream.Context())
			chippermetrics.CountConnectionCheckFail.Inc(1)

			toSend.Status = "Timeout"
			break receiveLoop
		default:
			req, suberr := cr.stream.Recv()

			if suberr != nil {
				err = suberr
				alog.Warn{
					"action":             "recv_audio",
					"status":             "unexpected_error",
					"svc":                CHECK,
					"error":              err,
					"frames_received":    frames,
					"frames_per_request": framesPerRequest,
					"device":             cr.device,
					"session":            cr.session,
				}.LogC(cr.stream.Context())
				checkRequestError(err)
				chippermetrics.CountConnectionCheckFail.Inc(1)
				chippermetrics.CountConnectionCheckError.Inc(1)
				toSend.Status = "Error"
				break receiveLoop
			}

			// POC-debug
			alog.Debug{
				"action":             "recv_audio",
				"status":             alog.StatusOK,
				"svc":                CHECK,
				"frame":              frames,
				"frames_per_request": framesPerRequest,
				"time_elapsed_ms":    durationMs(cr.reqTime),
				"device":             req.DeviceId,
				"session":            req.Session,
				"bytes_recv":         len(req.InputAudio),
			}.LogC(cr.stream.Context())

			frames += 1
			toSend.FramesReceived = frames
			if frames >= framesPerRequest {
				alog.Info{
					"action":             "connection_check",
					"status":             "success",
					"svc":                CHECK,
					"frames_received":    frames,
					"frames_per_request": framesPerRequest,
					"device":             cr.device,
					"session":            cr.session,
				}.LogC(cr.stream.Context())
				chippermetrics.CountConnectionCheckSuccess.Inc(1)
				toSend.Status = "Success"
				break receiveLoop
			}
		}
	}
	senderr := cr.stream.Send(&toSend)
	if senderr != nil {
		alog.Warn{"action": "send_client",
			"status":  alog.StatusError,
			"error":   senderr,
			"svc":     CHECK,
			"device":  cr.device,
			"session": cr.session,
			"msg":     "fail to send connection check response to client",
		}.LogC(cr.stream.Context())
		chippermetrics.CountConnectionCheckError.Inc(1)
		return senderr
	}
	return err
}
