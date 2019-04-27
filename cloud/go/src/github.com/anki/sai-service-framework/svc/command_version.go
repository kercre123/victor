package svc

import (
	"encoding/json"
	"fmt"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

func WithVersionCommand() ServiceConfig {
	return WithCommand("version", "Display program version information",
		func(cmd *cli.Cmd) {
			cmd.Spec = "[--format]"
			format := cmd.StringOpt("f format", "text",
				`Output format for version information. One of "text", "json", or "log"`)
			cmd.Action = func() {
				switch *format {
				case "text":
					fmt.Println(buildinfo.Info())
				case "log":
					logentry := alog.Info{
						"command": "version",
					}
					for k, v := range buildinfo.Info().Map() {
						logentry[k] = v
					}
					logentry.Log()
				case "json":
					js, _ := json.Marshal(buildinfo.Info())
					fmt.Println(string(js))
				default:
					Fail("Unknown version format '%s'", *format)
				}
			}
		})
}
