package blobstore

import (
	"fmt"
	"net/http"

	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func InfoCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("info", "Display metadata for a file from the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace] --id"

		namespace := cmd.StringOpt("n namespace", "cozmo", "Namespace to download blob from")
		id := cmd.StringOpt("i id", "", "ID of the blob to display")

		cmd.Action = func() {
			c := newClient(*cfg)

			resp, err := c.Info(*namespace, *id)
			if err != nil {
				cliutil.Fail("Error sending request: %v", err)
			}

			if resp.StatusCode != http.StatusOK {
				// a head request isn't going to have a JSON body; print the status instead
				cliutil.Fail("Unexpected response: %v", resp.Status)
			}

			// Print file metadata
			for k, v := range resp.Header {
				if len(v) > 0 {
					fmt.Printf("%s: %s\n", k, v[0])
				}
			}
		}
	})
}
