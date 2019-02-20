package main

import (
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"

	metrics "github.com/rcrowley/go-metrics"
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

	mutex  sync.Mutex
	robots []*robotSimulator

	sizeGauge      metrics.Gauge
	actionRegistry *actionMetricRegistry
}

func newRobotFleet(options *options) *robotFleet {
	return &robotFleet{
		distributedController: newDistributedController(*options.redisAddress),
		actionRegistry:        newActionMetricRegistry(),
	}
}

func (r *robotFleet) runSingleRobot(options *options, taskID, robotIndex int) {
	instanceOptions := options.createIdentity(r.distributedController, robotIndex, taskID)

	simulator, err := newRobotSimulator(r.actionRegistry, options, instanceOptions)

	simulator.logIfNoError(err, "framework", "new_robot", "New robot (id=%d) inside task (id=%d) created (start delay %v)",
		instanceOptions.robotID, taskID, instanceOptions.rampupDelay)
	if err != nil {
		return
	}

	r.mutex.Lock()
	r.robots = append(r.robots, simulator)
	r.mutex.Unlock()

	// At startup we go through the primary pairing sequence action
	simulator.addSetupAction("primary_association", instanceOptions.rampupDelay, simulator.testPrimaryPairingSequence)
	simulator.addTearDownAction("nop", instanceOptions.rampdownDelay, simulator.tearDownAction)

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

	simulator.logIfNoError(err, "framework", "stopping_robot", "Stopping robot (id=%d)", instanceOptions.robotID)

	simulator.stop()

	simulator.logIfNoError(err, "framework", "robot_stopped", "Robot (id=%d) stopped", instanceOptions.robotID)
}

func (r *robotFleet) runRobots(options *options) {
	taskID, err := r.distributedController.uniqueTaskID()
	if err == nil {
		// Note: As the Redis incrementer starts at 1, we apply an offset to start at 0
		taskID--
	} else {
		// We default to zero in case of error
		taskID = 0
	}

	setupMetrics(*options.wavefrontAddress, taskID, options.reportingInterval)

	r.sizeGauge = metrics.NewRegisteredFunctionalGauge("fleet_size", metrics.DefaultRegistry, func() int64 {
		var fleetSize int64 = 0
		for _, robot := range r.robots {
			if robot.state == SimulatorStarted {
				fleetSize++
			}
		}
		return fleetSize
	})

	if *options.enableDistributedControl {
		fmt.Printf("Task %d listening for external simulation commands\n", taskID)
		r.distributedController.forwardCommands(r)
	}

	var wg sync.WaitGroup

	for i := 0; i < *options.robotsPerProcess; i++ {
		wg.Add(1)

		go func(robotIndex int) {
			defer wg.Done()

			r.runSingleRobot(options, taskID, robotIndex)
		}(i)
	}

	waitForSignal()

	wg.Wait()
}

// Implement localController interface to propagate control to every robot instance
func (r *robotFleet) start() error {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	for _, robot := range r.robots {
		robot.start()
	}

	fmt.Printf("Started all %d robots\n", len(r.robots))

	return nil
}

func (r *robotFleet) stop() error {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	for _, robot := range r.robots {
		robot.stop()
	}

	fmt.Printf("Stopped all %d robots\n", len(r.robots))

	return nil
}

func (r *robotFleet) set(name, value string) {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	for _, robot := range r.robots {
		robot.set(name, value)
	}

	fmt.Printf("Set property %q to %q for all %d robots\n", name, value, len(r.robots))
}

func (r *robotFleet) quit() error {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	for _, robot := range r.robots {
		robot.quit()
	}

	fmt.Printf("Terminated all %d robots\n", len(r.robots))

	return nil
}
