package das

import (
	"fmt"
	"time"

	daslib "github.com/anki/sai-das-client/das"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/log"

	cli "github.com/jawher/mow.cli"
)

func CountEventsCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command(
		"countevents",
		"Count an entire day's worth of aggregated DAS events",
		func(cmd *cli.Cmd) {
			cmd.Spec = "--source-env --date --product"

			sourceEnv := cmd.StringOpt("s source-env", "", "Environment to load DAS data FROM")
			dateStr := cmd.StringOpt("d date", "", "The date to load. All aggregated data for all tables on this date will be loaded.")
			prodStr := cmd.StringOpt("p product", "", "Type of aggregated data to load. drive, od, ft, cozmo, cloud, vic, vicapp")

			cmd.Action = func() {

				//
				// Validate arguments
				//

				date, err := time.Parse("2006-01-02", *dateStr)
				if err != nil {
					cliutil.Fail("Failed to parse date str: %v", err)
				}

				product := *prodStr
				if !daslib.IsValidProduct(product) {
					cliutil.Fail("Invalid product: %v. Must be one of %v", product, daslib.AllProducts)
				}
				aggType := daslib.ProductToAggregateMap[product]

				//
				// Initialize DAS aggregate browser
				//

				browser, err := daslib.NewDASBrowser(*sourceEnv)
				if err != nil {
					cliutil.Fail("Failed to initialize DAS browser: %v", err)
				}

				//
				// Iterate through data
				//

				var count int
				err = browser.IterateAggregatesForDay(date, aggType, func(entry *daslib.LoaderNotification) error {

					// For each aggregate, download and decode it
					evts, err := browser.DownloadBundle(entry.Path, aggType)
					if err != nil {
						cliutil.Fail("Failed to download S3 bundle %v: %v", entry.Path, err)
					}

					alog.Info{
						"action": "count",
						"count":  len(evts),
						"total":  count,
						"path":   entry.Path,
					}.Log()

					count += len(evts)
					return nil
				})

				fmt.Println()
				fmt.Println()
				fmt.Println("  Date: ", *dateStr, "; Total Event Count: ", count)
				fmt.Println()
				fmt.Println()
			}
		})
}
