package blobstore

import (
	"fmt"

	"github.com/anki/sai-blobstore/client/blobstore"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

const (
	UploadTimeKey = "Srv-Upload-Time"
	BlobIDKey     = "Srv-Blob-Id"
)

var columns = []struct {
	Fmt      string
	KeyName  string
	DispName string
}{
	{"%-22s", UploadTimeKey, "Time"},
	{"%-40.38s", BlobIDKey, "Blob ID"},
}

func TailCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("tail", "Tail entries from the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace]"

		namespace := cmd.StringOpt("n namespace", "cozmo",
			"Namespace to monitor")

		cmd.Action = func() {
			c := newClient(*cfg)

			tailCfg := &blobstore.TailConfig{
				Namespace: *namespace,
			}

			fmt.Println("Namespace", *namespace)
			for _, col := range columns {
				fmt.Printf(col.Fmt, col.DispName)
			}
			fmt.Println("")

			err := c.Tail(tailCfg, func(matches []map[string]string) bool {
				for _, md := range matches {
					for _, col := range columns {
						fmt.Printf(col.Fmt, md[col.KeyName])
					}
					fmt.Println("")
				}
				return false
			})
			if err != nil {
				cliutil.Fail(err.Error())
			}
		}
	})
}
