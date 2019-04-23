package main

import (
	"anki/config"
	"anki/log"
	"anki/token/identity"
	"os"

	scli "github.com/anki/sai-go-util/cli"
	mcli "github.com/jawher/mow.cli"
)

func init() {
	scli.InitLogFlags("robot_fleet")
}

func main() {
	// init logging and make sure it gets cleaned up
	scli.SetupLogging()
	defer scli.CleanupAndExit()

	app := mcli.App("robot_simulator", "Robot cloud simulation tool")

	options := newFromEnvironment(app)

	if err := config.SetGlobal(*options.urlConfigFile); err != nil {
		log.Println("Could not load server config! This is not good!:", err)
	}

	// Enable client certs for non-vicos build
	identity.UseClientCert = true

	robotFleet := newRobotFleet(options)

	app.Action = func() {
		robotFleet.runRobots(options)
	}

	err := app.Run(os.Args)
	if err != nil {
		log.Println("Error starting simulator:", err)
	}
}
