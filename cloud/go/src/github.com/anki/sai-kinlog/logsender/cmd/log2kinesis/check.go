package main

import (
	"github.com/anki/sai-kinlog/logsender"
	cli "github.com/jawher/mow.cli"
)

func checkCommand(app *cli.Cli) {
	app.Command("check", "Check Kinesis connectivity", func(cmd *cli.Cmd) {
		cmd.Spec = "--stream-name"

		stream := cmd.String(cli.StringOpt{
			Name:   "stream-name",
			EnvVar: "LOG_KINESIS_STREAM",
			Desc:   "Kinesis stream name to test",
		})
		_ = stream

		cmd.Action = func() {
			_, err := logsender.New(*stream)
			if err != nil {
				fail("Failed to initialize Kinesis sender: %v", err)
			}
		}
	})
}
