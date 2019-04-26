package main

import (
	"os"

	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	"context"
	tc "github.com/anki/sai-chipper-voice/client/testclient"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"time"
)

func StreamFileCommand(cfg testClientCfg, authCfg tc.AuthConfig) svc.ServiceConfig {
	return svc.WithCommand("stream-file", "send streaming audio from file", func(cmd *cli.Cmd) {

		cmd.Spec = cfg.CommandSpec() + authCfg.CommandSpec() +
			" --audio  --noise " +
			" [--audio-encoding] [--single-utterance] "

		cfg.CommandInitialize(cmd)
		authCfg.CommandInitializ(cmd)

		audioFile := svc.StringOpt(cmd, "audio", "", "Name of audio file to stream")
		noiseFile := svc.StringOpt(cmd, "noise", "", "Name of noise file to stream after audio file")
		singleUtterance := svc.BoolOpt(cmd, "single-utterance", true, "Single utterance")
		encoding := svc.StringOpt(cmd, "audio-encoding", "opus", "Audio encoding (pcm, opus)")
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
				tc.WithSingleUtterance(*singleUtterance),
				tc.WithAudioEncoding(tc.Encoding[*encoding]),
				tc.WithAuthConfig(authCfg),
				tc.WithSkipDas(*cfg.skipDas),
				tc.WithSaveAudio(*cfg.saveAudio),
			)
			if err != nil {
				os.Exit(-1)
			}
			defer client.Conn.Close()

			d, err := tc.ReadFiles(*noiseFile, *audioFile)
			if err != nil {
				alog.Error{"action": "read_files", "error": err}.Log()
				os.Exit(-2)
			}

			if *cfg.mode == "command" {
				res, _ := client.StreamFromFile(d)
				if res != nil {
					printResult2(res, "Voice Command Stream-File")
				}
			} else if *cfg.mode == "kg" {
				res, _ := client.KnowledgeGraph(d)
				if res != nil {
					printResult2(res, "Knowledge Graph Stream-File")
				}
			} else {
				alog.Error{
					"action":     "stream_file",
					"error":      "invalid mode",
					"input_mode": *cfg.mode,
				}.Log()
			}
		}
	})
}
