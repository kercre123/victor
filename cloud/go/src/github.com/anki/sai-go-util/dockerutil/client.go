package dockerutil

import (
	"io"
	"os"

	"github.com/fsouza/go-dockerclient"
)

var (
	Logger           io.Writer
	DefaultEndpoint  = "unix:///var/run/docker.sock"
	DefaultClient, _ = NewClient(DefaultEndpoint)
)

func init() {
	if host := os.Getenv("DOCKER_HOST"); host != "" {
		DefaultClient, _ = NewClientFromEnv()
	}
}

// Client wraps the go-dockerclient Client type to add some convenience methods.
//
// It embeds that type so all properties and methods are supported.
type Client struct {
	*docker.Client
}

// NewClient creates a new Client object, wrapping the native
// go-dockerclient Client. See also NewClientFromEnv.
func NewClient(endpoint string) (*Client, error) {
	dc, err := docker.NewClient(endpoint)
	if err != nil {
		return nil, err
	}
	return &Client{dc}, nil
}

// NewClientFromEnv initializes a new Docker client object from
// environment variables. This is the best way to initialize a client
// on OS X, where communication with the Docker API is via a
// self-signed HTTPS endpoint, not a UNIX socket.
func NewClientFromEnv() (*Client, error) {
	dc, err := docker.NewClientFromEnv()
	if err != nil {
		return nil, err
	}
	return &Client{dc}, nil
}

// PullPublicIfRequired checks to see if the local docker installation has an image available
// and if not attempts to fetch it from the public docker registry.
//
// It sends any logging information to the Logger defined in this package (if any).
func (c *Client) PullPublicIfRequired(imageName string) error {
	img, _ := c.InspectImage(imageName)
	if img != nil {
		// already got it
		return nil
	}
	return c.PullImage(docker.PullImageOptions{
		Repository:   imageName,
		OutputStream: Logger,
	}, docker.AuthConfiguration{})
}

// CreateContainer is a convenience wrapper around Client.CreateContainer() that
// returns a wrapped Container type.
func (client *Client) CreateContainer(opts docker.CreateContainerOptions) (*Container, error) {
	container, err := client.Client.CreateContainer(opts)
	if err != nil {
		return nil, err
	}
	return &Container{container, client}, nil
}

// InspectContainer is a convience wrapp around Client.InspectContainer() that
// returns a wrapped Container type.
func (client *Client) InspectContainer(id string) (*Container, error) {
	container, err := client.Client.InspectContainer(id)
	if err != nil {
		return nil, err
	}
	return &Container{container, client}, nil
}

// StartNewContainer creates a new container, starts it and retrieves detailed information about it.
func (client *Client) StartNewContainer(containerOpts docker.CreateContainerOptions) (c *Container, err error) {
	c = &Container{client: client}
	c.Container, err = client.Client.CreateContainer(containerOpts)
	if err != nil {
		return nil, err
	}
	if err = client.Client.StartContainer(c.Container.ID, containerOpts.HostConfig); err != nil {
		return nil, err
	}
	c.Container, err = client.Client.InspectContainer(c.Container.ID)
	return c, nil
}
