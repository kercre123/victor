package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"

	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/strutil"
	"github.com/anki/sai-service-framework/svc"

	tc "github.com/anki/sai-chipper-voice/client/testclient"
)

type testClientCfg struct {
	serverAddr *string
	serverPort *int
	device     *string
	svc        *string
	mode       *string
	fw         *string
	insecure   *bool
	session    *string
	skipDas    *bool
	saveAudio  *bool
	ctx        *context.Context
}

func (c *testClientCfg) CommandSpec() string {
	return "[--addr] [--port] " +
		"[--device] [--svc] [--mode] [--firmware] " +
		"[--insecure] [--skip-das] [--save-audio] "
}

func (c *testClientCfg) CommandInitialize(cmd *cli.Cmd) {
	c.serverAddr = svc.StringOpt(cmd, "addr", tc.DefaultServer, "chipper server address")
	c.serverPort = svc.IntOpt(cmd, "port", tc.DefaultPort, "chipper server port")
	c.device = svc.StringOpt(cmd, "device", tc.DefaultDeviceId, "robot id")
	c.fw = svc.StringOpt(cmd, "firmware", tc.DefaultFW, "robot firmware version")
	c.svc = svc.StringOpt(cmd, "svc", "default", "intent-service to use (default, lex, goog)")
	c.mode = svc.StringOpt(cmd, "mode", "command", "robot mode (command, games, kg)")
	c.insecure = svc.BoolOpt(cmd, "insecure", false, "insecure mode, use true for local testing")
	c.skipDas = svc.BoolOpt(cmd, "skip-das", false, "skip saving data to DAS")
	c.saveAudio = svc.BoolOpt(cmd, "save-audio", false, "save audio")

	session, err := strutil.ShortLowerUUID()
	if err != nil {
		alog.Error{"action": "create_session_id", "status": "fail", "error": err}.Log()
		os.Exit(-1)
	}
	c.session = &session
}

func main() {
	alog.ToStdout()
	cfg := testClientCfg{}
	authCfg := tc.AuthConfig{}

	svc.ServiceMain("chipper-client", "Chipper test client",
		TextCommand(cfg, authCfg),
		StreamFileCommand(cfg, authCfg),
		StreamMicCommand(cfg, authCfg),
	)
}

func printResult2(res interface{}, resType string) {
	ejson, _ := json.MarshalIndent(res, "", "    ")
	fmt.Printf("%s Response:\n%s\n", resType, string(ejson))
}
