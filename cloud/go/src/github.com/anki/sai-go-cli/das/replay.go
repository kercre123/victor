package das

import (
	"fmt"
	"time"

	daslib "github.com/anki/sai-das-client/das"
	"github.com/anki/sai-das-client/events"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/log"

	cli "github.com/jawher/mow.cli"
)

func ReplayOneDayCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command(
		"replayday",
		"Retrieve an entire day's worth of aggregated DAS data and replay it to the raw DAS message queue",
		func(cmd *cli.Cmd) {
			cmd.Spec = "--source-env --date --product [--protocol-version] [--format] [--target-queue-url]"

			sourceEnv := cmd.StringOpt("s source-env", "", "Environment to load DAS data FROM")
			dateStr := cmd.StringOpt("d date", "", "The date to load. All aggregated data for all tables on this date will be loaded.")
			prodStr := cmd.StringOpt("p product", "", "Type of aggregated data to load. drive, od, ft, cozmo, cloud, vic, vicapp")
			protoVer := cmd.IntOpt("v protocol-version", 1, "DAS protocol version to use")
			formatStr := cmd.StringOpt("f format", "normal", "Marshalling format: normal or raw")
			targetQueueUrl := cmd.StringOpt("t target-queue-url", "", "Override for DAS SQS queue")

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

				cfg := *cfg

				queueURL := *targetQueueUrl
				if queueURL == "" {
					queueURL = cfg.Env.ServiceURLs.ByService(config.DAS)
					if queueURL == "" {
						cliutil.Fail("No config entry for das_queue_url and --target-queue-url not provided")
					}
				}

				if *protoVer != 1 && *protoVer != 2 {
					cliutil.Fail("Invalid DAS protocol version '%v'", *protoVer)
				}

				if *formatStr != "normal" && *formatStr != "raw" {
					cliutil.Fail("Invalid format '%s'", *formatStr)
				}

				//
				// Initialize DAS aggregate browser
				//

				browser, err := daslib.NewDASBrowser(*sourceEnv)
				if err != nil {
					cliutil.Fail("Failed to initialize DAS browser: %v", err)
				}

				//
				// Configure and initialize DAS client
				//

				var dascfg *daslib.ClientConfig
				if *protoVer == 1 {
					dascfg = daslib.NewLegacyConfig(product, queueURL)
				} else {
					dascfg = daslib.NewConfig(product, queueURL)
				}
				if *formatStr == "raw" {
					dascfg.Raw = true
				}
				dascfg.Synchronous = true

				client, err := daslib.NewBufferedClient(dascfg)
				if err != nil {
					cliutil.Fail("DAS client init failed: %v", err)
				}

				fmt.Println()
				fmt.Println()
				fmt.Println("Protocol Version: ", *protoVer)
				fmt.Println("Content-Type:     ", dascfg.ContentType())
				fmt.Println("Content-Encoding: ", dascfg.ContentEncoding())
				fmt.Println("Source Env:       ", *sourceEnv)
				fmt.Println("Product:          ", product)
				fmt.Println("Aggregate Type:   ", aggType)
				fmt.Println("Date:             ", date)
				fmt.Println()
				fmt.Println()

				//
				// Iterate through data
				//

				err = browser.IterateAggregatesForDay(date, aggType, func(entry *daslib.LoaderNotification) error {

					// For each aggregate, download and decode it
					evts, err := browser.DownloadBundle(entry.Path, aggType)
					if err != nil {
						cliutil.Fail("Failed to download S3 bundle %v: %v", entry.Path, err)
					}

					alog.Info{
						"action":      "download_bundle",
						"path":        entry.Path,
						"type":        aggType,
						"event_count": len(evts),
					}.Log()
					if dascfg.Raw {
						for i := range evts {
							e := evts[i].(*events.LegacyEvent)
							evts[i] = &events.RawLegacyEvent{LegacyEvent: *e}
						}
					}

					// Then send all the events to the target
					// environment, allowing the client to buffer as
					// necessary.
					msgIds, err := client.SendEvents(evts)
					if err != nil {
						alog.Error{
							"action": "send_events",
							"status": "error",
							"error":  err,
						}.Log()
					} else {
						alog.Info{
							"action":        "send_events",
							"status":        "ok",
							"event_count":   len(evts),
							"message_count": len(msgIds),
						}.Log()
					}
					return nil
				})
				if err != nil {
					cliutil.Fail("Failed to iterate aggregates: %v", err)
				}
				client.Stop()
			}
		})
}
