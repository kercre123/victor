package stream

import (
	"anki/log"
	"anki/util"
	"clad/cloud"
	"errors"

	"github.com/anki/sai-chipper-voice/client/chipper"
	"github.com/google/uuid"
)

func (strm *Streamer) connect() error {
	var jwtToken string
	var err error
	var tokenTime float64
	if strm.opts.tokener != nil {
		tokenTime = util.TimeFuncMs(func() {
			jwtToken, err = strm.opts.tokener()
		})
	}
	if strm.opts.requireToken {
		if jwtToken == "" && err == nil {
			err = errors.New("token required, got empty string")
		}
		if err != nil {
			strm.receiver.OnError(cloud.ErrorType_Token, err)
			return err
		}
	}

	sessionID := uuid.New().String()[:16]
	connectTime := util.TimeFuncMs(func() {
		strm.conn, strm.stream, err = strm.openStream(jwtToken, sessionID)
	})
	if err != nil {
		log.Println("Error creating Chipper:", err)
		return err
	}

	// signal to engine that we got a connection; the _absence_ of this will
	// be used to detect server timeout errors
	strm.receiver.OnStreamOpen(sessionID)

	logVerbose("Received hotword event", strm.opts.mode, "created session", sessionID, "in",
		int(connectTime), "ms (token", int(tokenTime), "ms)")
	return nil
}

func (strm *Streamer) openStream(jwtToken string, sessionID string) (*chipper.Conn, chipper.Stream, error) {
	opts := platformOpts
	if jwtToken != "" {
		opts = append(opts, chipper.WithCredentials(util.GrpcMetadata(jwtToken)))
	}
	opts = append(opts, chipper.WithSessionID(sessionID))
	conn, err := chipper.NewConn(strm.ctx, strm.opts.url, strm.opts.secret, opts...)
	if err != nil {
		log.Println("Error getting chipper connection:", err)
		strm.receiver.OnError(cloud.ErrorType_Connecting, err)
		return nil, nil, err
	}
	var stream chipper.Stream
	if strm.opts.mode == cloud.StreamType_KnowledgeGraph {
		stream, err = conn.NewKGStream(strm.ctx, strm.opts.streamOpts.StreamOpts)
	} else {
		stream, err = conn.NewIntentStream(strm.ctx, strm.opts.streamOpts)
	}
	if err != nil {
		strm.receiver.OnError(cloud.ErrorType_NewStream, err)
		return nil, nil, err
	}
	return conn, stream, err
}
