package stream

import (
	"anki/config"
	"anki/log"
	"anki/robot"
	"anki/util"
	"clad/cloud"
	"crypto/tls"
	"errors"
	"net/http"
	"strings"
	"time"

	"github.com/gwatts/rootcerts"

	"github.com/anki/sai-chipper-voice/client/chipper"
	"github.com/google/uuid"
	"google.golang.org/grpc/credentials"
)

const (
	HeadRequestTimeout = 8 * time.Second
)

func (strm *Streamer) connect() error {
	if strm.opts.checkOpts != nil {
		// for connection check, first try the OTA CDN with no tls
		otaBase := strings.Split(config.Env.OTA, ":")[0]
		// construct a HTTP URL from OTA address (something like `ota-cdn.anki.com:443`)
		otaURL := "http://" + otaBase
		if req, err := http.NewRequest("HEAD", otaURL, nil); err != nil {
			log.Println("Error creating CDN server http head request:", err)
			strm.receiver.OnError(cloud.ErrorType_Connectivity, err)
			return err
		} else if resp, err := http.DefaultClient.Do(req.WithContext(strm.ctx)); err != nil {
			log.Println("Error requesting head of CDN server:", err)
			strm.receiver.OnError(cloud.ErrorType_Connectivity, err)
			return err
		} else {
			resp.Body.Close()
			log.Println("Successfully dialed CDN")
		}

		// for connection check, next try a simple https connection to our OTA CDN
		httpsClient := &http.Client{
			Transport: &http.Transport{
				TLSClientConfig: &tls.Config{
					RootCAs: rootcerts.ServerCertPool(),
				},
			},
		}
		// construct a HTTPS URL from OTA address (something like `ota-cdn.anki.com:443`)
		otaURL = "https://" + otaBase
		if req, err := http.NewRequest("HEAD", otaURL, nil); err != nil {
			log.Println("Error creating CDN server https head request:", err)
			strm.receiver.OnError(cloud.ErrorType_TLS, err)
			return err
		} else if resp, err := httpsClient.Do(req.WithContext(strm.ctx)); err != nil {
			log.Println("Error requesting head of CDN server over https:", err)
			strm.receiver.OnError(cloud.ErrorType_TLS, err)
			return err
		} else {
			resp.Body.Close()
			log.Println("Successfully dialed CDN over https")
		}
	}

	var creds credentials.PerRPCCredentials
	var err error
	var tokenTime float64
	if strm.opts.tokener != nil {
		tokenTime = util.TimeFuncMs(func() {
			creds, err = strm.opts.tokener.Credentials()
		})
	}
	if strm.opts.requireToken {
		if creds == nil && err == nil {
			err = errors.New("token required, got empty string")
		}
		if err != nil {
			strm.receiver.OnError(cloud.ErrorType_Token, err)
			return err
		}
	}

	sessionID := uuid.New().String()[:16]
	connectTime := util.TimeFuncMs(func() {
		strm.conn, strm.stream, err = strm.openStream(creds, sessionID)
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

func (strm *Streamer) openStream(creds credentials.PerRPCCredentials, sessionID string) (*chipper.Conn, chipper.Stream, error) {
	opts := platformOpts
	if grpcOpts := util.CommonGRPC(); grpcOpts != nil {
		opts = append(opts, chipper.WithGrpcOptions(grpcOpts...))
	}
	if creds != nil {
		opts = append(opts, chipper.WithCredentials(creds))
	}
	opts = append(opts, chipper.WithSessionID(sessionID))
	opts = append(opts, chipper.WithFirmwareVersion(robot.OSVersion()))
	opts = append(opts, chipper.WithBootID(robot.BootID()))
	conn, err := chipper.NewConn(strm.ctx, strm.opts.url, strm.opts.secret, opts...)
	if err != nil {
		log.Println("Error getting chipper connection:", err)
		strm.receiver.OnError(cloud.ErrorType_Connecting, err)
		return nil, nil, err
	}
	var stream chipper.Stream
	if strm.opts.checkOpts != nil {
		stream, err = conn.NewConnectionStream(strm.ctx, *strm.opts.checkOpts)
	} else if strm.opts.mode == cloud.StreamType_KnowledgeGraph {
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
