package dockerutil

import (
	"fmt"
	"net"
	"net/url"
	"os"
	"runtime"
	"strings"
	"time"

	"github.com/fsouza/go-dockerclient"
)

// Container wraps the go-dockerclient Container type to add some convenience methods.
//
// It embeds that type so all properties and methods are supported.
type Container struct {
	*docker.Container
	client *Client
}

// Stop stops the running container, optionally removing it afterwards.
func (c *Container) Stop(del bool, killTimeout uint, removeVolumes bool) error {
	if err := c.client.StopContainer(c.Container.ID, killTimeout); err != nil {
		return err
	}
	if del {
		// do delete
		if err := c.client.RemoveContainer(
			docker.RemoveContainerOptions{
				ID:            c.ID,
				RemoveVolumes: removeVolumes,
			}); err != nil {
			return err
		}
	}
	return nil
}

// IpAddress returns the IP address associated with the container, if
// any. If we're running under OS X, the returned address will be that
// of the docker host, as the container's address is not directly
// accessible.
func (c *Container) IpAddress() string {
	if c.Container.NetworkSettings == nil {
		return ""
	} else if runtime.GOOS == "darwin" {
		dockerhost := os.Getenv("DOCKER_HOST")
		if dockerhost == "" {
			return ""
		}
		u, err := url.Parse(dockerhost)
		if err != nil {
			return ""
		}
		if !strings.Contains(u.Host, ":") {
			return u.Host
		}
		host, _, err := net.SplitHostPort(u.Host)
		if err != nil {
			return ""
		}
		return host
	}
	return c.Container.NetworkSettings.IPAddress
}

// Port returns the primary exposed port of the container. If we're
// running under OS X, this will be the mapped port on the Docker
// host's network address. If the container is running under Linux,
// this will be the exposed port on the container's network address.
func (c *Container) Port() string {
	if c.Container.NetworkSettings == nil {
		return ""
	}
	for ep, _ := range c.Container.Config.ExposedPorts {
		if runtime.GOOS == "darwin" {
			for p, _ := range c.NetworkSettings.Ports {
				if ep == p {
					return c.NetworkSettings.Ports[p][0].HostPort
				}
			}
		} else {
			return strings.Split(string(ep), "/")[0]
		}
	}
	return ""
}

// Addr returns a complete network address with IP address and exposed
// port for connecting to the primary service exposed by a container.
func (c *Container) Addr() string {
	return net.JoinHostPort(c.IpAddress(), c.Port())
}

// WaitForContainer waits until the container's primary exposed port
// is listening and available. See WaitForListeningPort().
func (c *Container) WaitForContainer() error {
	return c.WaitForListeningPort(c.Addr())
}

// WaitForListeningPort waits for the container to be available by
// attempting to connect to the specified address and port. An error
// will be returned if the connection is not accepted before the
// timeout of 30 seconds.
func (c *Container) WaitForListeningPort(addr string) error {
	var t net.Conn
	var err error
	fmt.Printf("Waiting for container (%s)...", addr)
	start := time.Now()
	tryUntil := start.Add(30 * time.Second)
	for time.Now().Before(tryUntil) {
		t, err = net.Dial("tcp", addr)
		if err == nil {
			t.Close()
			fmt.Printf("Connected after %.1f seconds.\n", time.Since(start).Seconds())
			return nil
		}
		time.Sleep(100 * time.Millisecond)
		fmt.Printf(".")
	}
	fmt.Println("Failed: ", err)
	return err
}
