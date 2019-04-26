package das

import (
	"math/rand"
	"time"

	"github.com/anki/sai-das-client/das"
	"github.com/anki/sai-das-client/events"
	"github.com/anki/sai-das-simulator"
	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"
	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/log"

	cli "github.com/jawher/mow.cli"
)

const (
	pingEventName = "sim.ping"
)

func SimPingCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command(
		"ping",
		"Send a single sim.ping DAS event",
		func(cmd *cli.Cmd) {
			cmd.Spec = "--product [--target-queue-url]"

			prodStr := cmd.StringOpt("p product", "",
				"DAS product schema to use. One of od, ft, cozmo, vic, vicapp, cloud")
			targetQueueUrl := cmd.StringOpt("t target-queue-url", "",
				"Override for DAS SQS queue")

			cmd.Action = func() {

				//
				// Set and validate parameters
				//

				cfg := *cfg

				product := *prodStr
				if !das.IsValidProduct(product) {
					cliutil.Fail("Invalid product: %v. Must be one of %v", product, das.AllProducts)
				}

				queueURL := *targetQueueUrl
				if queueURL == "" {
					queueURL = cfg.Env.ServiceURLs.ByService(config.DAS)
					if queueURL == "" {
						cliutil.Fail("No config entry for das_queue_url and --target-queue-url not provided")
					}
				}

				//
				// Create the DAS config appropriately per product,
				// and make it synchronous so the event is sent
				// immediately.
				//

				var dasConfig *das.ClientConfig
				switch product {
				case das.ProductDrive, das.ProductOverdrive, das.ProductFoxtrot, das.ProductCozmo:
					dasConfig = das.NewLegacyConfig(product, queueURL)
				default:
					dasConfig = das.NewConfig(product, queueURL)
				}
				dasConfig.Synchronous = true

				dasClient, err := das.NewBufferedClient(dasConfig)
				if err != nil {
					cliutil.Fail("Error initializing DAS client: %s", err)
				}

				//
				// Send the event
				//

				e := makePingEvent(product)
				msgId, err := dasClient.SendEvent(e)
				if err != nil {
					cliutil.Fail("Error sending ping event: %s", err)
				} else {
					alog.Info{
						"action":         "send_ping",
						"status":         alog.StatusOK,
						"sqs_message_id": msgId,
					}.Log()
				}
			}
		})
}

func makePingEvent(product string) events.Event {
	var (
		info = buildinfo.Info()
		seq  = int64(0)
		ts   = time.Now().UTC().UnixNano() / 1e6
	)

	switch product {
	case das.ProductOverdrive, das.ProductFoxtrot, das.ProductCozmo:
		return &events.RawLegacyEvent{
			LegacyEvent: events.LegacyEvent{
				Messv:      2,
				AppRun:     sim.NewId(),
				Seq:        seq,
				Ts:         ts,
				Level:      das.LevelDebug,
				Event:      pingEventName,
				Sval:       sim.NewId(),
				Data:       sim.NewId(),
				Phys:       "", // TBD
				Unit:       sim.NewId(),
				ActionUnit: sim.NewId(),
				App:        info.Commit[:8],
				Product:    product,
				Platform:   "ios",
				SessionId:  sim.NewId(),
				PlayerId:   sim.NewId(),
				ProfileId:  sim.NewId(),
				Group:      "test-group",
				Duser:      "what-is-duser",
				Phone:      "iPhone",
				Game:       sim.NewId(),
				Lobby:      sim.NewId(),
				Tag:        "tag",
				User:       sim.NewId(),
			},
		}

	case das.ProductVictorRobot:
		return &events.VictorRobotEvent{
			Source:       info.Product,
			Event:        pingEventName,
			Ts:           ts,
			Seq:          seq,
			Level:        das.LevelDebug,
			AppRunId:     sim.NewId(),
			RobotId:      sim.NewId(),
			RobotVersion: info.Commit[:8],
			FeatureRunId: sim.NewId(),
			S1:           sim.NewId(),
			S2:           sim.NewId(),
			S3:           sim.NewId(),
			S4:           sim.NewId(),
			I1:           rand.Int63n(128),
			I2:           rand.Int63n(128),
			I3:           rand.Int63n(128),
			I4:           rand.Int63n(128),
		}

	case das.ProductVictorApp:
		return &events.VictorAppEvent{
			Source:       info.Product,
			AppRunId:     sim.NewId(),
			Event:        pingEventName,
			Ts:           ts,
			Seq:          seq,
			Level:        das.LevelDebug,
			Platform:     "ios",
			PhoneModel:   "test",
			PhoneOS:      "test",
			Unit:         sim.NewId(),
			AppVersion:   info.Commit[:8],
			ProfileId:    sim.NewId(),
			RobotId:      sim.NewId(),
			ConnectionId: sim.NewId(),
			FeatureRunId: sim.NewId(),
			S1:           sim.NewId(),
			S2:           sim.NewId(),
			S3:           sim.NewId(),
			S4:           sim.NewId(),
			I1:           rand.Int63n(128),
			I2:           rand.Int63n(128),
			I3:           rand.Int63n(128),
			I4:           rand.Int63n(128),
		}

	case das.ProductCloud:
		return &events.CloudEvent{
			Source:     info.Product,
			SequenceId: sim.NewId(),
			Event:      pingEventName,
			Ts:         ts,
			Seq:        seq,
			Level:      das.LevelDebug,
			ProfileId:  sim.NewId(),
			RobotId:    sim.NewId(),
			S1:         sim.NewId(),
			S2:         sim.NewId(),
			S3:         sim.NewId(),
			S4:         sim.NewId(),
			I1:         rand.Int63n(128),
			I2:         rand.Int63n(128),
			I3:         rand.Int63n(128),
			I4:         rand.Int63n(128),
		}
	}
	return nil
}
