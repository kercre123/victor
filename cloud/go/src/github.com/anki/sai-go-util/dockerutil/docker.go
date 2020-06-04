// Copyright 2014 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

/*
Package dockerutil provides some convience wrappers around the go-dockerclient package.
*/
package dockerutil

import (
	"archive/tar"
	"bytes"
	"fmt"
	"os"
	"syscall"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/fsouza/go-dockerclient"
)

// ContainerConfig defines the parameters for launching a Docker
// container.
type ContainerConfig struct {
	// Image is the name of the Docker image to be started. It will be
	// pulled from docker hub if necessary, or built, if Dockerfile or
	// Init are specified.
	Image string

	// Dockerfile is a custom Dockerfile to build a custom container
	// image from. At this time a custom Dockerfile cannot add
	// additional files to the image, but it can run configuration
	// commands during build. Only one of Dockerfile or Init should be
	// specified.
	Dockerfile string

	// Init specifies a handler function for optionally initializing
	// the docker image. This can be used to build a custom image, for
	// example, with more control than using the Dockerfile
	// parameter. Only one of Dockerfile or Init should be specified.
	Init func(*Client, *ContainerConfig) error

	// Post is a Docker port spec for the primary listening port and
	// protocol to be exposed, e.g. "8000/tcp".
	Port string

	// Env specifies a set of environment variables to pass to the
	// container, if supplied.
	Env []string

	// Args specifies a set of command line args to be passed to the
	// container invocation, if supplied.
	Args []string
}

func (cfg *ContainerConfig) Build() error {
	if cfg.Init == nil && cfg.Dockerfile == "" {
		// Nothing to do
		return nil
	}
	if cfg.Image == "" {
		return fmt.Errorf("Image name must be specified")
	}
	if cfg.Init != nil && cfg.Dockerfile != "" {
		return fmt.Errorf("Custom Init and Dockerfile both specified")
	}

	// make sure we don't race against other test processes when
	// checking for and creating our custom image.
	fl := Lock(cfg.Image)
	defer fl.Unlock()

	if cfg.Init != nil {
		alog.Info{
			"action": "build_container",
			"status": "build_from_init",
			"image":  cfg.Image,
		}.Log()
		if err := cfg.Init(DefaultClient, cfg); err != nil {
			return err
		}
	} else if cfg.Dockerfile != "" {
		alog.Info{
			"action": "build_container",
			"status": "build_from_dockerfile",
			"image":  cfg.Image,
		}.Log()

		t := time.Now()
		inputbuf, outputbuf := bytes.NewBuffer(nil), bytes.NewBuffer(nil)
		tr := tar.NewWriter(inputbuf)
		tr.WriteHeader(&tar.Header{Name: "Dockerfile", Size: int64(len(cfg.Dockerfile)), ModTime: t, AccessTime: t, ChangeTime: t})
		tr.Write([]byte(cfg.Dockerfile))
		tr.Close()
		opts := docker.BuildImageOptions{
			Name:         cfg.Image,
			InputStream:  inputbuf,
			OutputStream: outputbuf,
		}

		return DefaultClient.BuildImage(opts)
	}
	return nil
}

type ContainerConfigs map[string]*ContainerConfig

// Containers is a map defining a set of named container
// configurations available for use.
var Containers = ContainerConfigs{
	"postgres": &ContainerConfig{
		Image: "postgres",
		Port:  "5432/tcp",
	},
	"dynamo": &ContainerConfig{
		Image: "peopleperhour/dynamodb",
		Port:  "8000/tcp",
	},
	"s3": &ContainerConfig{
		Image: "minio/minio",
		Port:  "9000/tcp",
		Env: []string{
			"MINIO_BROWSER=off",
			"MINIO_UPDATE=off",
		},
		Args: []string{
			"server",
			"/home/minio",
		},
	},
	"sqs": &ContainerConfig{
		Image: "fingershock/elasticmq",
		Port:  "9324/tcp",
	},
	"kinesis": &ContainerConfig{
		Image: "instructure/kinesalite",
		Port:  "4567/tcp",
		Args: []string{
			"--deleteStreamMs", "0",
		},
	},
	"mysql": &ContainerConfig{
		Image: "mysql:5.6.34",
		Port:  "3306/tcp",
		Init: func(client *Client, config *ContainerConfig) error {
			img, _ := client.InspectImage(mysqlUtf8Image)
			if img == nil {
				if err := buildMysqlImage(config); err != nil {
					return err
				}
			}
			return nil
		},
	},
}

// StartContainer starts a named container configuration, with an
// optional set of environment variables, and an optional override for
// the image name.
func StartContainer(name string, env []string, imageOverride string) (*Container, error) {
	// Check that the named configuration exists
	if config, ok := Containers[name]; !ok {
		return nil, fmt.Errorf("Unknown container name '%s'", name)
	} else {
		return StartContainerWithConfig(config, env, imageOverride)
	}
}

func StartContainerWithConfig(config *ContainerConfig, env []string, imageOverride string) (*Container, error) {
	// Download the image if necessary
	if config.Image != "" && config.Dockerfile == "" {
		alog.Info{
			"action": "start_container",
			"status": "pull_image",
			"image":  config.Image,
		}.Log()
		if err := DefaultClient.PullPublicIfRequired(config.Image); err != nil {
			return nil, err
		}
	}

	// Build custom image, if needed
	if err := config.Build(); err != nil {
		return nil, err
	}

	image := config.Image
	if imageOverride != "" {
		image = imageOverride
	}

	envToUse := []string{}
	envToUse = append(envToUse, env...)
	envToUse = append(envToUse, config.Env...)

	// Start the container
	container, err := DefaultClient.StartNewContainer(docker.CreateContainerOptions{
		Config: &docker.Config{
			Image:        image,
			ExposedPorts: map[docker.Port]struct{}{docker.Port(config.Port): {}},
			Env:          envToUse,
			Cmd:          config.Args,
		},
		HostConfig: &docker.HostConfig{
			PublishAllPorts: true,
		},
	})

	if err != nil {
		return nil, err
	}

	return container, nil
}

func StartMysqlContainer(rootPassword, databaseName string) (*Container, error) {
	return StartContainer("mysql",
		[]string{
			"MYSQL_ROOT_PASSWORD=" + rootPassword,
			"MYSQL_DATABASE=" + databaseName,
		}, mysqlUtf8Image)
}

func StartPostgresContainer(rootPassword string) (*Container, error) {
	return StartContainer("postgres",
		[]string{
			"POSTGRES_PASSWORD=" + rootPassword,
		}, "")
}

func StartDynamoContainer() (*Container, error) {
	return StartContainer("dynamo",
		nil, "")
}

func StartS3Container(accessKey, secretKey string) (*Container, error) {
	return StartContainer("s3",
		[]string{
			"MINIO_ACCESS_KEY=" + accessKey,
			"MINIO_SECRET_KEY=" + secretKey,
		}, "")
}

func StartSQSContainer() (*Container, error) {
	return StartContainer("sqs", []string{}, "")
}

func StartKinesisContainer() (*Container, error) {
	return StartContainer("kinesis", []string{}, "")
}

// ----------------------------------------------------------------------
// Custom MySQL Image
// ----------------------------------------------------------------------

const mysqlUtf8Image = "mysql-utf8mb4"

// buildMysqlImages applies a custom my.cnf to the upstream
// mysql image and saves it.
func buildMysqlImage(config *ContainerConfig) error {
	t := time.Now()
	dockerFile := fmt.Sprintf(mysqlDockerFile, config.Image)
	inputbuf, outputbuf := bytes.NewBuffer(nil), bytes.NewBuffer(nil)
	tr := tar.NewWriter(inputbuf)
	tr.WriteHeader(&tar.Header{Name: "Dockerfile", Size: int64(len(dockerFile)), ModTime: t, AccessTime: t, ChangeTime: t})
	tr.Write([]byte(dockerFile))
	tr.WriteHeader(&tar.Header{Name: "my.cnf", Mode: 0444, Size: int64(len(mysqlCfg)), ModTime: t, AccessTime: t, ChangeTime: t})
	tr.Write([]byte(mysqlCfg))
	tr.Close()
	opts := docker.BuildImageOptions{
		Name:         "mysql-utf8mb4",
		InputStream:  inputbuf,
		OutputStream: outputbuf,
	}

	err := DefaultClient.BuildImage(opts)
	return err
}

type flock struct {
	f *os.File
}

func Lock(name string) *flock {
	f := &flock{}
	f.f, _ = os.Create("/tmp/dockerutil-" + name + ".lock")
	syscall.Flock(int(f.f.Fd()), syscall.LOCK_EX)
	return f
}

func (f flock) Unlock() {
	if f.f != nil {
		syscall.Flock(int(f.f.Fd()), syscall.LOCK_UN)
		f.f.Close()
		f.f = nil
	}
}

const mysqlDockerFile = `
FROM %s
ADD my.cnf /etc/mysql/conf.d/golang.cnf
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["mysqld"]
`

const mysqlCfg = `
[mysqld]
bind-address=0.0.0.0
console=1
general_log=1
general_log_file=/dev/stdout
log_error=/dev/stderr
character-set-client-handshake = FALSE
character-set-server = utf8mb4
collation-server = utf8mb4_unicode_ci
`
