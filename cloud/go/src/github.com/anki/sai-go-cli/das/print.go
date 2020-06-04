package das

import (
	"fmt"
	"io"
	"log"
	"os"

	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func PrintCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("print", "Print the contents of DAS data files", func(cmd *cli.Cmd) {
		cmd.Spec = "[PATHS...]"

		paths := cmd.StringsArg("PATHS", nil, "Filenames to decode - Defaults to stdin")

		cmd.Action = func() {
			if len(*paths) == 0 {
				decodeFile("stdin", os.Stdin)
				return
			}
			for _, path := range *paths {
				f, err := os.Open(path)
				if err != nil {
					log.Fatal("Failed to open file %s: %v", path, err)
				}
				decodeFile(path, f)
				f.Close()
			}
		}
	})
}

func decodeFile(path string, r io.Reader) {
	mgr, err := newMaybeGzipReader(r)
	if err != nil {
		log.Fatal("Failed to read %s: %v", path, err)
	}

	if mgr.isGzipped {
		fmt.Println("Filename  : ", path)
		io.Copy(os.Stdout, mgr)
	} else {
		msg, err := decodeSQS(mgr)
		if err != nil {
			log.Fatal("Failed to decode %s: %v", path, err)
		}
		dumpMsg(path, msg)
	}
	fmt.Println()
}
