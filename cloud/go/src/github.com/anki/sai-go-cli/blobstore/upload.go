package blobstore

import (
	"os"
	"path/filepath"
	"strings"

	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/jawher/mow.cli"
)

var userMetadataPrefix = "usr-"

func UploadCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("upload", "Upload a file to the blobstore", func(cmd *cli.Cmd) {
		cmd.Spec = "[--namespace] [--id] --meta... --filename"

		namespace := cmd.StringOpt("n namespace", "cozmo", "Namespace to upload blob into")
		filename := cmd.StringOpt("f filename", "", "Filename to upload")
		id := cmd.StringOpt("i id", "", "ID to assign to uploaded blob; defaults to the base filename of the uploaded file")
		meta := cmd.StringsOpt("m meta", nil, "key=value metadata pairs; must provide at least one")

		cmd.Action = func() {
			c := newClient(*cfg)

			if *id == "" {
				*id = filepath.Base(*filename)
			}

			md := make(map[string]string)
			for _, m := range *meta {
				kv := strings.SplitN(m, "=", 2)
				if len(kv) != 2 {
					cliutil.Fail("metadata pairs must be in key=val format")
				}
				if !strings.HasPrefix(strings.ToLower(kv[0]), userMetadataPrefix) {
					cliutil.Fail("metadata keys must start with \"%s\"", userMetadataPrefix)
				}
				md[kv[0]] = kv[1]
			}

			f, err := os.Open(*filename)
			if err != nil {
				cliutil.Fail("Failed to open file to upload: %v", err)
			}
			apiutil.DefaultHandler(c.Upload(*namespace, f, *id, md))
		}
	})
}
