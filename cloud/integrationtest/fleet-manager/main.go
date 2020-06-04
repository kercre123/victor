package main

import (
	"fmt"
	"os"
	"time"

	cli "github.com/jawher/mow.cli"
)

type duration time.Duration

func (d *duration) Set(v string) error {
	parsed, err := time.ParseDuration(v)
	if err != nil {
		return err
	}
	*d = duration(parsed)
	return nil
}

func (d *duration) String() string {
	duration := time.Duration(*d)
	return duration.String()
}

func main() {
	app := cli.App("fleet-manager", "Command the virtual robot fleet")

	app.Spec = "[-a=<redis-address>] [-c=<cluster-name>] [-r=<region>]"

	redisAddress := app.StringOpt("a redis-address", "redis.loadtest:6379", "Redis server host and port")
	clusterName := app.StringOpt("c cluster-name", "load_test", "AWS ECS / Fargate cluster name")
	awsRegion := app.StringOpt("r region", "us-west-2", "AWS Region")

	app.Command("deploy", "Deploy robot fleet tasks", func(cmd *cli.Cmd) {
		manager := newFleetManager(*redisAddress, *clusterName, *awsRegion)

		cmd.Spec = "[-t=<timeout>] [-i=<interval>]"

		var timeout, interval duration

		// Set option defaults
		timeout.Set("10m")
		interval.Set("5s")

		cmd.VarOpt("t timeout", &timeout, "Deployment timeout (in time.Duration string format)")
		cmd.VarOpt("i interval", &interval, "Deployment status polling interval (in time.Duration string format)")

		cmd.Action = func() {
			manager.deployRobotFleet(time.Duration(timeout), time.Duration(interval))
		}
	})

	app.Command("start", "Start robot fleet", func(cmd *cli.Cmd) {
		cmd.Action = func() {
			manager := newFleetManager(*redisAddress, *clusterName, *awsRegion)
			manager.controlRobotFleet("start")
		}
	})

	app.Command("stop", "Stop robot fleet", func(cmd *cli.Cmd) {
		cmd.Action = func() {
			manager := newFleetManager(*redisAddress, *clusterName, *awsRegion)
			manager.controlRobotFleet("stop")
		}
	})

	app.Command("quit", "Quit robot fleet", func(cmd *cli.Cmd) {
		cmd.Action = func() {
			manager := newFleetManager(*redisAddress, *clusterName, *awsRegion)
			manager.controlRobotFleet("quit")
		}
	})

	app.Command("set", "Set robot fleet statistical parameter", func(cmd *cli.Cmd) {
		manager := newFleetManager(*redisAddress, *clusterName, *awsRegion)

		cmd.Spec = "KEY VALUE"

		key := cmd.StringArg("KEY", "", "Name of statistical parameter")
		value := cmd.StringArg("VALUE", "", "Value of statistical parameter")

		cmd.Action = func() {
			manager.commandFleet(fmt.Sprintf("set:%s=%s", *key, *value))
		}
	})

	app.Run(os.Args)
}
