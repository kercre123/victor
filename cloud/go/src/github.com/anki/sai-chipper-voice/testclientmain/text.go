package main

import (
	"context"
	"os"
	"time"

	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-service-framework/svc"

	tc "github.com/anki/sai-chipper-voice/client/testclient"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

func TextCommand(cfg testClientCfg, authCfg tc.AuthConfig) svc.ServiceConfig {
	return svc.WithCommand("text", "send text request", func(cmd *cli.Cmd) {
		cfg := testClientCfg{}
		authCfg := tc.AuthConfig{}

		cmd.Spec = cfg.CommandSpec() + authCfg.CommandSpec() + " --text "

		cfg.CommandInitialize(cmd)
		authCfg.CommandInitializ(cmd)
		text := svc.StringOpt(cmd, "text", "", "text query")
		ctx, _ := context.WithTimeout(context.Background(), 10*time.Second)

		if *cfg.mode == "kg" {
			alog.Error{"error": "no kg mode in for text query"}.Log()
			os.Exit(-1)
		}
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
				tc.WithSkipDas(*cfg.skipDas),
			)
			if err != nil {
				os.Exit(-3)
			}
			defer client.Conn.Close()

			res := client.TextIntent(*text)
			if res != nil {
				printResult2(res, "Voice Command Text")
			}

		}

	})
}
