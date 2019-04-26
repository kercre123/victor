package httpcapture

import (
	"compress/gzip"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"

	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
	"github.com/skratchdot/open-golang/open"
)

func OpenCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("open", "Download and open an HTTP capture file from the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace] [--opener] [--dir] --id"
		namespace := cmd.StringOpt("n namespace", "sai-dev-capture",
			"Namespace to download capture from")
		id := cmd.StringOpt("i id", "", "Request ID of the capture to download")
		dir := cmd.StringOpt("d dir", os.TempDir(), "Directory to save file to")
		opener := cmd.StringOpt("o opener", "",
			`Program to open the .har file with; defaults to system default program.  `+
				`Specify "none" to skip opening`)

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

			fn := filepath.Join(*dir, *id+".har")
			f, err := os.Create(fn)
			if err != nil {
				cliutil.Fail("Failed to open file %q for write: %v", fn, err)
			}

			// Decompress the file as it's read
			gzr, err := gzip.NewReader(resp.Body)
			if err != nil {
				cliutil.Fail("Failed to open response for decompression: %v", err)
			}

			// Download the file
			fmt.Printf("\nDownloading %s bytes...", resp.Header.Get("Srv-Blob-Size"))
			n, err := io.Copy(f, gzr)
			if err != nil {
				cliutil.Fail("Error during download after %d bytes: %#v", n, err)
			}
			fmt.Printf("Downloaded %s (actual body size: %d bytes)\n", fn, n)
			f.Close()

			switch *opener {
			case "none":
			case "":
				fmt.Println("Opening", fn)
				if err := open.Start(fn); err != nil {
					fmt.Println("Failed to open %q: %s", fn, err)
				}
			default:
				fmt.Println("Opening", fn)
				if err := open.StartWith(fn, *opener); err != nil {
					fmt.Println("Failed to open %q with %q: %s", fn, *opener, err)
				}
			}
		}
	})

}
