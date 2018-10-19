package main

import (
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"
)

var waitChannel chan struct{}
var signalChannel chan os.Signal

func init() {
	waitChannel = make(chan struct{})
	signalChannel := make(chan os.Signal, 1)
	signal.Notify(signalChannel, os.Interrupt, syscall.SIGTERM)
}

func waitForSignal() {
	// wait for SIGTERM signal
	<-signalChannel

	// signal robot simulators to stop
	close(waitChannel)
}

type robotFleet struct {
	distributedController *distributedController
	robots                []*robotSimulator
}

func newRobotFleet(options *options) *robotFleet {
	return &robotFleet{distributedController: newDistributedController(*options.redisAddress)}
}

func (r *robotFleet) runSingleRobot(options *options) {
	instanceOptions := options.createIdentity(r.distributedController)

	simulator, err := newRobotSimulator(options, instanceOptions)
	simulator.logIfNoError(err, "main", "New robot (id=%d) created", instanceOptions.testID)
	if err != nil {
		return
	}

	r.robots = append(r.robots, simulator)

	// At startup we go through the primary pairing sequence action
	simulator.addSetupAction(simulator.testPrimaryPairingSequence)
	simulator.addTearDownAction(simulator.tearDownAction)

	// After that we periodically run the following actions
	simulator.addPeriodicAction("heart_beat", options.heartBeatInterval, options.heartBeatStdDev, simulator.heartBeat)
	simulator.addPeriodicAction("jdocs", options.jdocsInterval, options.jdocsStdDev, simulator.testJdocsReadAndWriteSettings)
	simulator.addPeriodicAction("logging", options.logCollectorInterval, options.logCollectorStdDev, simulator.testLogCollector)
	simulator.addPeriodicAction("token_refresh", options.tokenRefreshInterval, options.tokenRefreshStdDev, simulator.testTokenRefresh)
	simulator.addPeriodicAction("mic_connection_check", options.connectionCheckInterval, options.connectionCheckStdDev, simulator.testMicConnectionCheck)

	if !*options.enableDistributedControl {
		simulator.start()
	}

	// wait for stop signal
	<-waitChannel

	simulator.logIfNoError(err, "main", "Stopping robot (id=%d)", instanceOptions.testID)

	simulator.stop()

	simulator.logIfNoError(err, "main", "Robot (id=%d) stopped", instanceOptions.testID)
}

func (r *robotFleet) runRobots(options *options) {
	if *options.enableDistributedControl {
		fmt.Println("Listening for external simulation commands")
		r.distributedController.forwardCommands(r)
	}

	var wg sync.WaitGroup

	for i := 0; i < *options.robotsPerProcess; i++ {
		wg.Add(1)

		go func() {
			defer wg.Done()

			r.runSingleRobot(options)
		}()
	}

	waitForSignal()

	wg.Wait()
}

// Implement localController interface to propagate control to every robot instance
func (r *robotFleet) start() {
	for _, robot := range r.robots {
		robot.start()
	}

	fmt.Printf("Started all %d robots\n", len(r.robots))
}

func (r *robotFleet) stop() {
	for _, robot := range r.robots {
		robot.stop()
	}

	fmt.Printf("Stopped all %d robots\n", len(r.robots))
}

func (r *robotFleet) set(name, value string) {
	for _, robot := range r.robots {
		robot.set(name, value)
	}

	fmt.Printf("Set property %q to %q for all %d robots\n", name, value, len(r.robots))
}

func (r *robotFleet) quit() {
	for _, robot := range r.robots {
		robot.quit()
	}

	fmt.Printf("Terminated all %d robots\n", len(r.robots))
}
