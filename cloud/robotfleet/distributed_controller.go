package main

import (
	"fmt"
	"strings"
	"syscall"
	"time"

	"github.com/go-redis/redis"
)

const (
	arrivalTimeKeyPrefix   = "robot:arrivalTime"
	departureTimeKeyPrefix = "robot:departureTime"
	taskIDKey              = "task_id"
	remoteControlChannel   = "load_test_control"
)

type robotIdentityProvider interface {
	uniqueTaskID() (int, error)
	arrivalTime(robotID int) (time.Duration, error)
	departureTime(robotID int) (time.Duration, error)
}

// This interface needs to be implemented by the remote controlled struct instance
// (i.e. the robot simulator)
type localController interface {
	start() error
	stop() error
	set(name, value string)
	quit() error
}

// This struct handles incoming commands from Redis pub-sub and dispacthes them to
// the localController
type distributedController struct {
	*redis.Client

	localController localController
	pubsub          *redis.PubSub
}

func newDistributedController(address string) *distributedController {
	client := redis.NewClient(&redis.Options{Addr: address})
	return &distributedController{
		Client: client,
		pubsub: client.Subscribe(remoteControlChannel),
	}
}

func (c *distributedController) uniqueTaskID() (int, error) {
	id, err := c.Client.Incr(taskIDKey).Result()
	return int(id), err
}

func (c *distributedController) arrivalTime(robotID int) (time.Duration, error) {
	delayStr, err := c.Client.Get(fmt.Sprintf("%s:%d", arrivalTimeKeyPrefix, robotID)).Result()
	if err != nil {
		return 0, err
	}

	return time.ParseDuration(delayStr)
}

func (c *distributedController) departureTime(robotID int) (time.Duration, error) {
	delayStr, err := c.Client.Get(fmt.Sprintf("%s:%d", departureTimeKeyPrefix, robotID)).Result()
	if err != nil {
		return 0, err
	}

	return time.ParseDuration(delayStr)
}

func (c *distributedController) forwardCommands(localController localController) {
	c.localController = localController

	go func() {
		for message := range c.pubsub.Channel() {
			args := strings.Split(message.Payload, ":")

			switch args[0] {
			case "start":
				c.localController.start()
			case "stop":
				c.localController.stop()
			case "set":
				keyValueParts := strings.Split(args[1], "=")
				if len(keyValueParts) == 2 {
					c.localController.set(keyValueParts[0], keyValueParts[1])
				} else {
					fmt.Println("Received invalid key/value pair for set command:", args[1])
				}
			case "quit":
				c.pubsub.Close()
				c.localController.quit()
				signalChannel <- syscall.SIGINT
			default:
				fmt.Printf("Received unexpected remote controle command: %q\n", args[0])
			}
		}
	}()
}

// only used for testing purposes
func (c *distributedController) sendCommand(command string) error {
	if err := c.Publish(remoteControlChannel, command).Err(); err != nil {
		return err
	}

	return nil
}

func (c *distributedController) close() error {
	return c.pubsub.Close()
}
