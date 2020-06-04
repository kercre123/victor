package main

import (
	"fmt"
	"os"

	"github.com/anki/sai-go-cli/accounts"
	"github.com/anki/sai-go-cli/ankival"
	"github.com/anki/sai-go-cli/audit"
	"github.com/anki/sai-go-cli/blobstore"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-cli/das"
	"github.com/anki/sai-go-cli/httpcapture"
	"github.com/anki/sai-go-cli/jdocs"
	"github.com/anki/sai-go-cli/vrscmd"
	"github.com/anki/sai-go-util/buildinfo"
	acli "github.com/anki/sai-go-util/cli"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/log"
	"github.com/jawher/mow.cli"
)

const (
	defaultEnvironment = "dev"
	defaultSessionName = "default"
	nilSession         = "none"
	nilAppKey          = "none"
)

func init() {
	acli.InitLogFlags("sai_go_cli")
}

func main() {
	var cfg *config.Config

	alog.ToStdout()

	app := cli.App("sai-go-cli", "Interface with Anki backend services")

	app.Spec = "[--cfgfile] [--env] [--session-name] [--session-token] [--appkey | --appkey-name] [--dump-request] [--dump-response]"

	cfgFilename := app.StringOpt("c cfgfile", "", "Path to config file")
	envName := app.StringOpt("e env", defaultEnvironment, "Environment to interact with")
	sessionName := app.StringOpt("s session-name", defaultSessionName, "Named session to interact with.  Set to \"none\" for no session.")
	sessionToken := app.StringOpt("session-token", "", "Override session token")
	appKeyName := app.StringOpt("appkey-name", "", "Use named app key from active environment (one of admin, user, support, request, cron or service - set to none for no app key)")
	appKey := app.StringOpt("appkey", "", "Use explicit app key.")
	dumpRequest := app.StringOpt("dump-request", "", "Enable request debug output.  Set to one of \"headers\", \"body\" or \"both\"")
	dumpResponse := app.StringOpt("dump-response", "", "Enable response debug output.  Set to one of \"headers\", \"body\" or \"both\"")

	app.Command("accounts", "Use the Anki Accounts API", func(c *cli.Cmd) {
		accounts.InitializeCommands(c, &cfg)
	})

	app.Command("ankival", "Use the Anki Key-Value Store API", func(c *cli.Cmd) {
		ankival.InitializeCommands(c, &cfg)
	})

	app.Command("audit", "Use the Anki Accounts Audit Log API", func(c *cli.Cmd) {
		audit.InitializeCommands(c, &cfg)
	})

	app.Command("blobstore", "Use the Anki Blobstore API", func(c *cli.Cmd) {
		blobstore.InitializeCommands(c, &cfg)
	})

	app.Command("das", "Commands to work with DAS data", func(c *cli.Cmd) {
		das.InitializeCommands(c, &cfg)
	})

	app.Command("httpcapture", "Access HTTP Capture data from the blobstore API", func(c *cli.Cmd) {
		httpcapture.InitializeCommands(c, &cfg)
	})

	app.Command("jdocs", "Use the Anki Jdocs API", func(c *cli.Cmd) {
		jdocs.InitializeCommands(c, &cfg)
	})

	app.Command("virtualrewards", "Use the Virtual RewardsAPI", func(c *cli.Cmd) {
		vrscmd.InitializeCommands(c, &cfg)
	})

	app.Command("version", "Display program version information", func(c *cli.Cmd) {
		c.Action = func() {
			fmt.Println(buildinfo.Info())
		}
	})

	app.Before = func() {
		var err error
		if *sessionName == nilSession {
			*sessionName = ""
		}

		if cfg, err = config.Load(*cfgFilename, *cfgFilename != "", *envName, *sessionName); err != nil {
			cliutil.Fail("Failed to read configuration file: %v", err)
		}

		if *sessionName == "" {
			cfg.Session.Token = ""
		} else if *sessionToken != "" {
			cfg.Session.Token = *sessionToken
		}

		if *appKeyName == nilAppKey {
			cfg.Session.AppKey = ""

		} else if *appKeyName != "" {
			if err := cfg.SetActiveAppKeyName(*appKeyName); err != nil {
				cliutil.Fail("Invalid app key: %v", err)
			}
		}

		if *appKey != "" {
			cfg.Session.AppKey = *appKey
		}

		if *dumpRequest != "" {
			if err := cfg.SetDumpRequest(*dumpRequest); err != nil {
				cliutil.Fail("Invalid setting for dump-request: %v", err)
			}
		}
		if *dumpResponse != "" {
			if err := cfg.SetDumpResponse(*dumpResponse); err != nil {
				cliutil.Fail("Invalid setting for dump-response: %v", err)
			}
		}
	}

	// I had to add a call to envconfig.DefaultConfig.Flags.Parse(),
	// because the common code in sai-go-util to retrieve AWS
	// credentials uses envconfig
	// -alpern
	if len(os.Args) > 2 {
		envconfig.DefaultConfig.Flags.Parse(os.Args[2:])
	}
	acli.SetupLogging()

	app.Run(os.Args)
}
