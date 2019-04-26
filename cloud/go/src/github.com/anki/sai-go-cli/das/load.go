package das

import (
	"fmt"
	"time"

	daslib "github.com/anki/sai-das-client/das"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"

	cli "github.com/jawher/mow.cli"
)

func LoadOneDayCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command(
		"loadday",
		"Send the loader notifications to load an entire day's worth of aggregated DAS data",
		func(cmd *cli.Cmd) {
			cmd.Spec = "--source-env --date --type"

			sourceEnv := cmd.StringOpt("s source-env", "", "Environment to load DAS data FROM")
			dateStr := cmd.StringOpt("d date", "", "The date to load. All aggregated data for this date will be loaded.")
			typeStr := cmd.StringOpt("t type", "", "Type of aggregate data to load. oddas, odgeoagg, cozmodas, cozmogeoagg, etc...")

			cmd.Action = func() {
				date, err := time.Parse("2006-01-02", *dateStr)
				if err != nil {
					cliutil.Fail("Failed to parse date str: %v", err)
				}

				dataType := *typeStr
				if !daslib.IsValidAggregateType(dataType) {
					cliutil.Fail("Invalid data type: %v. Must be one of %v", dataType, daslib.AllAggregateTypes)
				}

				fmt.Printf("Loading %v / %v / %v\n", *sourceEnv, *typeStr, date)

				browser, err := daslib.NewDASBrowser(*sourceEnv)
				if err != nil {
					cliutil.Fail("Failed to initialize DAS browser: %v", err)
				}

				cfg := *cfg

				queueURL := cfg.Env.ServiceURLs.ByService(config.RedshiftLoaderQueue)
				if queueURL == "" {
					cliutil.Fail("No config entry for redshift_loader_queue_url")
				}

				lc, err := daslib.NewLoaderClient(queueURL)
				if err != nil {
					cliutil.Fail("Redshift Loader client init failed: %v", err)
				}

				browser.IterateAggregatesForDay(date, dataType, func(entry *daslib.LoaderNotification) error {
					id, err := lc.SendNotification(&daslib.LoaderNotification{
						AggregateType: entry.AggregateType,
						Path:          entry.Path,
						Bytes:         entry.Bytes,
					})
					if err != nil {
						cliutil.Fail("Redshift Loader client failed to send notification: %v", err)
					} else {
						fmt.Printf("Sent to queue %s, message id %s: %v\n", queueURL, id, entry)
					}
					return nil
				})
			}
		})
}
