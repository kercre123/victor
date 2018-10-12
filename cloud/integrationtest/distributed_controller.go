package main

import (
	"fmt"
	"strings"

	"github.com/go-redis/redis"
)

const (
	testIDKey            = "test_id"
	remoteControlChannel = "load_test_control"
)

// This interface needs to be implemented by the remote controlled struct instance
// (i.e. the robot simulator)
type localController interface {
	start()
	stop()
	quit()
}

type State int

const (
	Unknown = iota
	Initialized
	Started
	Stopped
	Terminated
)

// This struct handles incoming commands from Redis pub-sub and dispacthes them to
// the localController
type distributedController struct {
	*redis.Client

	state           State
	localController localController
	pubsub          *redis.PubSub
}

func newDistributedController(address string) *distributedController {
	client := redis.NewClient(&redis.Options{Addr: address})
	return &distributedController{
		Client: client,
		pubsub: client.Subscribe(remoteControlChannel),
		state:  Initialized,
	}
}

func (c *distributedController) provideUniqueTestID() (int, error) {
	id, err := c.Client.Incr(testIDKey).Result()
	return int(id), err
}

func (c *distributedController) forwardCommands(localController localController) {
	c.localController = localController

	go func() {
		for message := range c.pubsub.Channel() {
			args := strings.Split(message.Payload, ":")

			switch args[0] {
			case "start":
				c.localController.start()
				c.state = Started
			case "stop":
				c.localController.stop()
				c.state = Stopped
			case "quit":
				c.pubsub.Close()
				c.localController.quit()
				c.state = Terminated
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
