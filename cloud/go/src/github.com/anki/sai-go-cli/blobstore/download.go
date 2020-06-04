package blobstore

import (
	"fmt"
	"io"
	"net/http"
	"os"

	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

func DownloadCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("download", "Download a file from the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace] [--target-name] --id"

		namespace := cmd.StringOpt("n namespace", "cozmo", "Namespace to download blob from")
		id := cmd.StringOpt("i id", "", "ID of the blob to download")
		targetName := cmd.StringOpt("t target-name", "", "Name to give download file; defaults to the blob id")

		cmd.Action = func() {
			c := newClient(*cfg)

			resp, err := c.Download(*namespace, *id)
			if err != nil {
				cliutil.Fail("Error sending request: %v", err)
			}
			if resp.StatusCode != http.StatusOK {
				apiutil.DefaultJsonHandler(resp, err)
				return
			}

			// Open target file
			if *targetName == "" {
				*targetName = *id
			}
			f, err := os.Create(*targetName)
			if err != nil {
				cliutil.Fail("Failed to open file %q for write: %v", *targetName, err)
			}
			defer f.Close()

			// Print file metadata
			for k, v := range resp.Header {
				if len(v) > 0 {
					fmt.Printf("%s: %s\n", k, v[0])
				}
			}

			// Download the file
			fmt.Printf("\nDownloading %s bytes...", resp.Header.Get("Srv-Blob-Size"))
			n, err := io.Copy(f, resp.Body)
			if err != nil {
				cliutil.Fail("Error during download after %d bytes: %#v", n, err)
			}
			fmt.Printf("Done (actual body size: %d bytes)\n", n)
		}
	})
}
