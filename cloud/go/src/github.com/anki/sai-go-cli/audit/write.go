package audit

import (
	"fmt"
	"os"

	"github.com/anki/sai-accounts-audit/audit"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func WriteCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("write", "Write a new audit log entry", func(cmd *cli.Cmd) {
		cmd.Spec = "--account_id --action [--principal]"

		accountId := cmd.StringOpt("account_id", "", "")
		action := cmd.StringOpt("action", "", "Audit action")
		principal := cmd.StringOpt("principal", os.Getenv("SAI_CLI_PRINCIPAL"), "")
		// requestId := cmd.StringOpt("request_id", "", "")
		level := cmd.StringOpt("level", "", "")
		comment := cmd.StringOpt("comment", "", "")

		cmd.Action = func() {
			cfg := *cfg
			c := newClient(cfg)

			if *principal != "" {
				if cfg.Session != nil {
					cliutil.Fail("Use --session=none if supplying a principal")
				}
			}

			log := &audit.Record{
				AccountID: *accountId,
				Action:    *action,
				Comment:   *comment,
			}

			if *level != "" {
				log.Level = *level
			}

			if *principal != "" {
				p, err := audit.ParsePrincipal(*principal)
				if err != nil {
					cliutil.Fail("Invalid principal: %v", err)
				}
				log.Principal = p
			}

			id, err := c.WriteRecord(log)
			if err != nil {
				cliutil.Fail("Write failed: %v", err)
			}
			fmt.Println(id)
		}
	})
}
