package main

import (
	"bytes"
	"context"
	"encoding/binary"
	"log"
	"os"
	"time"

	"github.com/gordonklaus/portaudio"
	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	tc "github.com/anki/sai-chipper-voice/client/testclient"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

const (
	inputChannels = 1
	sampleRate    = 16000
	bufSize       = sampleRate * 2 * .120 // 100ms
)

func StreamMicCommand(cfg testClientCfg, authCfg tc.AuthConfig) svc.ServiceConfig {
	return svc.WithCommand("stream-mic", "send streaming audio request", func(cmd *cli.Cmd) {

		cmd.Spec = cfg.CommandSpec() + authCfg.CommandSpec() + " [--single-utterance] "

		cfg.CommandInitialize(cmd)
		authCfg.CommandInitializ(cmd)

		singleUtterance := svc.BoolOpt(cmd, "single-utterance", true, "Single utterance")
		ctx, _ := context.WithTimeout(context.Background(), 10*time.Second)

		cmd.Action = func() {

			client, err := tc.NewTestClient(
				ctx,
				*cfg.serverAddr,
				*cfg.serverPort,
				tc.WithDeviceId(*cfg.device),
				tc.WithSession(*cfg.session),
				tc.WithLangCode(pb.LanguageCode_ENGLISH_US),
				tc.WithService(tc.Services[*cfg.svc]),
				tc.WithInsecure(*cfg.insecure),
				tc.WithFirmware(*cfg.fw),
				tc.WithMode(tc.Modes[*cfg.mode]),
				tc.WithAuthConfig(authCfg),
				tc.WithSingleUtterance(*singleUtterance),
				tc.WithSkipDas(*cfg.skipDas),
				tc.WithSaveAudio(*cfg.saveAudio),
			)
			if err != nil {
				os.Exit(-1)
			}
			defer client.Conn.Close()

			portaudio.Initialize()
			defer portaudio.Terminate()

			audioStream := make(chan []byte, 10)
			done := make(chan struct{}, 1)
			waitc := make(chan struct{}, 1)
			audioStreamDone := make(chan struct{}, 1)

			go readAudio(audioStream, done, waitc, audioStreamDone)

			if *cfg.mode == "command" || *cfg.mode == "game" {
				res, _ := client.StreamFromMic(audioStream, audioStreamDone)
				if res != nil {
					printResult2(res, "Voice Command Stream-Mic")
				}
			} else if *cfg.mode == "kg" {
				res, _ := client.KnowledgeGraphFromMic(audioStream, audioStreamDone)
				if res != nil {
					printResult2(res, "Knowledge Graph Stream-Mic")
				}
			} else {
				alog.Error{
					"action":     "stream_mic",
					"error":      "invalid mode",
					"input_mode": *cfg.mode,
				}.Log()
			}
			done <- struct{}{}
			<-waitc

		}
	})
}

func readAudio(audioStream chan<- []byte, done chan struct{}, waitc chan struct{}, audioDone chan struct{}) {
	defer close(waitc)

	timeout := time.NewTimer(6 * time.Second)
	defer timeout.Stop()

	buf := make([]int16, sampleRate*0.120)
	outbuf := new(bytes.Buffer)

	flush := func() {
		audioStream <- outbuf.Bytes()
		outbuf = new(bytes.Buffer)
	}

	stream, err := portaudio.OpenDefaultStream(inputChannels, 0, float64(sampleRate), len(buf), buf)
	if err != nil {
		alog.Error{
			"action": "open_portaudio_stream",
			"status": alog.StatusFail,
			"error":  err,
			"msg":    "stream open failed",
		}.Log()
		os.Exit(-2)
	}
	defer stream.Close()

	if err := stream.Start(); err != nil {
		log.Fatal("Failed to start stream", err)
		alog.Error{
			"action": "start_portaudio_stream",
			"status": alog.StatusFail,
			"error":  err,
			"msg":    "Failed to start stream",
		}.Log()
		os.Exit(-3)
	}

	alog.Info{"action": "***** START_TALKING *****"}.Log()
	for {
		select {
		case <-done:
			alog.Info{"action": "closing_mic"}.Log()
			return
		case <-timeout.C:
			alog.Info{"action": "read_audio_timeout"}.Log()
			audioDone <- struct{}{}
			return
		default:
			for outbuf.Len() < bufSize {
				if err := stream.Read(); err != nil {
					if err != nil {
						alog.Error{
							"action": "read_portaudio_stream",
							"status": alog.StatusError,
							"error":  err,
							"msg":    "stream read failed",
						}.Log()
						return
					}
				}
				binary.Write(outbuf, binary.LittleEndian, buf)
			}
			flush()
		}

	}
}
