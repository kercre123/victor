package ankival

import (
	"strings"

	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func SendTxnCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("send-txn", "Submit a transaction", func(cmd *cli.Cmd) {
		cmd.Spec = "[--user-id] [--txn-id] KEYVALS..."

		userId := cmd.StringOpt("u user-id", "", "User ID of profile to send txn to")
		txnId := cmd.StringOpt("t txn-id", "", "Transaction id")
		keyvals := cmd.StringsArg("KEYVALS", nil, "key=val operations to submit")

		cmd.Action = func() {
			c := newClient(*cfg)

			if len(*keyvals) == 0 {
				cliutil.Fail("No operations supplied")
			}

			ops := make(map[string]string)
			for _, kv := range *keyvals {
				parts := strings.SplitN(kv, "=", 2)
				if len(parts) != 2 {
					cliutil.Fail("KEYVALS must be key=value pairs")
				}
				k, v := strings.TrimSpace(parts[0]), strings.TrimSpace(parts[1])
				ops[k] = v
			}

			apiutil.DefaultHandler(c.SubmitTxn(*userId, *txnId, ops))
		}
	})
}
