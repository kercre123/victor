package das

import (
	"log"
	"os"
	"path/filepath"
	"regexp"

	"github.com/anki/sai-go-cli/config"
	cli "github.com/jawher/mow.cli"
)

func GrepCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command("grep", "Search archived DAS data", func(cmd *cli.Cmd) {
		cmd.Spec = "[-r] [PATHS...] -- PATTERN"

		recurse := cmd.BoolOpt("r", false, "Recurse directories")
		paths := cmd.StringsArg("PATHS", nil, "Paths to search")
		pattern := cmd.StringArg("PATTERN", "", "Pattern to match (Go regular expression)")

		cmd.Action = func() {
			re, err := regexp.Compile(*pattern)
			if err != nil {
				log.Fatal("Invalid pattern: %v", err)
			}
			for _, path := range *paths {
				if *recurse {
					filepath.Walk(path, func(path string, info os.FileInfo, err error) error {
						if info.IsDir() {
							return nil
						}
						if err != nil {
							log.Println("Failed to open %s: %v", path, err)
							return nil
						}
						dasGrep(path, re)
						return nil
					})
				} else {
					dasGrep(path, re)
				}
			}
		}
	})
}
