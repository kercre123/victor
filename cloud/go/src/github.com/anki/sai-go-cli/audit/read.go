package audit

import (
	"encoding/json"
	"fmt"

	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func ReadCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("read", "Read a single complete audit log entry", func(cmd *cli.Cmd) {
		cmd.Spec = "--id"

		id := cmd.StringOpt("i id", "", "Log entry id")

		cmd.Action = func() {
			c := newClient(*cfg)

			log, err := c.GetRecord(*id)
			if err != nil {
				cliutil.Fail("Command failed: %v", err)
			}
			js, _ := json.MarshalIndent(*log, "", "\t")
			fmt.Println(string(js))
		}
	})
}
