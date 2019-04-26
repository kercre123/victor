package das

import (
	"time"

	"github.com/anki/sai-das-client/das"
	"github.com/anki/sai-das-simulator"
	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/log"

	"github.com/anki/sai-go-cli/cliutil"
	"github.com/anki/sai-go-cli/config"

	cli "github.com/jawher/mow.cli"
)

var (
	commit   string
	queueURL string
	interval = time.Millisecond * 500
)

func SimHeartbeatCommand(parent *cli.Cmd, cfg **config.Config) {
	parent.Command(
		"heartbeat",
		"Send periodic sim.heartbeat DAS events",
		func(cmd *cli.Cmd) {
			cmd.Spec = "[--interval] [--product...] [--target-queue-url]"

			intervalStr := cmd.StringOpt("i interval", "500ms", "A duration expression indicating how often to generate heartbeat events. E.g. 1s, 500ms, 200ms, etc...")
			productsOpt := cmd.StringsOpt("p product", nil, "Products to generate heartbeats for. One or more of od, ft, cozmo, cloud, vic, vicapp, or all")
			targetQueueUrl := cmd.StringOpt("t target-queue-url", "", "Override for DAS SQS queue")

			cmd.Action = func() {
				commit = buildinfo.Info().Commit[:8]

				//
				// Validate the arguments
				//

				cfg := *cfg

				queueURL = *targetQueueUrl
				if queueURL == "" {
					queueURL = cfg.Env.ServiceURLs.ByService(config.DAS)
					if queueURL == "" {
						cliutil.Fail("No config entry for das_queue_url and --target-queue-url not provided")
					}
				}

				var err error
				interval, err = time.ParseDuration(*intervalStr)
				if err != nil {
					cliutil.Fail("Error parsing interval: %s", err)
				}

				sims := []*sim.Simulator{}

				for _, prod := range *productsOpt {
					if prod == "all" {
						sims = []*sim.Simulator{
							makeOverdrive(), makeFoxtrot(), makeCozmo(), makeCloud(), makeVictorRobot(), makeVictorApp(),
						}
						break
					}
					sims = append(sims, makeSimulator(prod))
				}

				for _, sim := range sims {
					alog.Info{
						"action": "simulator_start",
						"sim":    sim.Name,
					}.Log()
					sim.Start()
				}

				sims[0].Wait()
			}
		})
}

func makeSimulator(p string) *sim.Simulator {
	switch p {
	case das.ProductOverdrive:
		return makeOverdrive()
	case das.ProductFoxtrot:
		return makeFoxtrot()
	case das.ProductCozmo:
		return makeCozmo()
	case das.ProductVictorRobot:
		return makeVictorRobot()
	case das.ProductVictorApp:
		return makeVictorApp()
	case das.ProductCloud:
		return makeCloud()
	default:
		cliutil.Fail("Unknown product '%s'", p)
	}
	return nil
}

func makeCloud() *sim.Simulator {
	s, err := sim.NewSimulator(
		"cloud",
		sim.NewCloudHeartbeatGenerator(&sim.Service{
			Name:    buildinfo.Info().Product,
			Id:      sim.NewId(),
			Version: commit,
		}),
		&sim.SimulatorConfig{
			ClientConfig: das.NewConfig(das.ProductCloud, queueURL),
			Interval:     interval,
		})

	if err != nil {
		cliutil.Fail("Error initializing simulator: %s", err)
	}
	return s
}

func makeVictorRobot() *sim.Simulator {
	s, err := sim.NewSimulator(
		"victor",
		sim.NewVictorRobotHeartbeatGenerator(&sim.Robot{
			SerialNumber: sim.NewId(),
			Version:      commit,
		}),
		&sim.SimulatorConfig{
			ClientConfig: das.NewConfig(das.ProductVictorRobot, queueURL),
			Interval:     interval,
		})

	if err != nil {
		cliutil.Fail("Error initializing simulator: %s", err)
	}
	return s
}

func makeVictorApp() *sim.Simulator {
	s, err := sim.NewSimulator(
		"victor_app",
		sim.NewVictorAppHeartbeatGenerator(
			&sim.App{
				Product: das.ProductVictorApp,
				Version: commit,
				Source:  "companion",
			},
			sim.NewDevice(sim.PlatformIos)),
		&sim.SimulatorConfig{
			ClientConfig: das.NewConfig(das.ProductVictorApp, queueURL),
			Interval:     interval,
		})

	if err != nil {
		cliutil.Fail("Error initializing simulator: %s", err)
	}
	return s
}

func makeOverdrive() *sim.Simulator {
	s, err := sim.NewSimulator(
		"overdrive",
		sim.NewLegacyHeartbeatGenerator(
			&sim.App{
				Product: das.ProductOverdrive,
				Version: commit,
			},
			sim.NewDevice(sim.PlatformAndroid),
		),
		&sim.SimulatorConfig{
			ClientConfig: das.NewLegacyConfig(das.ProductOverdrive, queueURL),
			Interval:     interval,
		})

	if err != nil {
		cliutil.Fail("Error initializing simulator: %s", err)
	}
	return s
}

func makeFoxtrot() *sim.Simulator {
	s, err := sim.NewSimulator(
		"foxtrot",
		sim.NewLegacyHeartbeatGenerator(
			&sim.App{
				Product: das.ProductFoxtrot,
				Version: commit,
			},
			sim.NewDevice(sim.PlatformIos),
		),
		&sim.SimulatorConfig{
			ClientConfig: das.NewLegacyConfig(das.ProductFoxtrot, queueURL),
			Interval:     interval,
		})

	if err != nil {
		cliutil.Fail("Error initializing simulator: %s", err)
	}
	return s
}

func makeCozmo() *sim.Simulator {
	s, err := sim.NewSimulator(
		"cozmo",
		sim.NewLegacyHeartbeatGenerator(
			&sim.App{
				Product: das.ProductCozmo,
				Version: commit,
			},
			sim.NewDevice(sim.PlatformIos),
		),
		&sim.SimulatorConfig{
			ClientConfig: das.NewLegacyConfig(das.ProductCozmo, queueURL),
			Interval:     interval,
		})

	if err != nil {
		cliutil.Fail("Error initializing simulator: %s", err)
	}
	return s
}
