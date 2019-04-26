package dockerutil

import (
	"fmt"
	"strings"

	"github.com/anki/sai-go-util"
	"github.com/stretchr/testify/suite"
)

// ----------------------------------------------------------------------
// Docker test suite
// ----------------------------------------------------------------------

// Suite provides a base type to use for constructing test suites that
// depend on docker container. Concrete test suite types must set the
// Start function.
type Suite struct {
	suite.Suite

	// Container is a pointer to the Container structure, which will
	// be assigned after SetupSuite() successfully starts the
	// container.
	Container *Container

	// Start is a function which launches a container, returning the
	// container and an optional error. Each concrete docker test
	// suite must set the Start member before propagating the
	// SetupSuite() call, and should call one of the container
	// starting functions provided by dockerutil.
	//
	// See S3Suite for a concrete example.
	Start func() (*Container, error)
}

func (s *Suite) SetupSuite() {
	container, err := s.Start()
	if err != nil {
		s.T().Fatalf("Failed to start docker container: %s", err)
	}

	err = container.WaitForContainer()
	if err != nil {
		s.T().Fatalf("Failed to start docker container: %s", err)
	}

	s.Container = container
}

var (
	DefaultTerminateAction       = "PURGE"
	DefaultContainerTimeout uint = 10
)

func (s *Suite) TearDownSuite() {
	terminateAction := DefaultTerminateAction
	util.SetFromEnv(&terminateAction, "DOCKER_TERMINATE_ACTION")
	terminateAction = strings.ToUpper(terminateAction)

	if s.Container != nil {
		switch terminateAction {
		case "PURGE":
			fmt.Println("Stopping & Deleting Container " + s.Container.Container.ID)
			s.Container.Stop(true, DefaultContainerTimeout, true)
		case "STOP":
			fmt.Println("Stopping Container " + s.Container.Container.ID)
			s.Container.Stop(false, DefaultContainerTimeout, true)
		default:
			fmt.Printf("Leaving container %s intact at IP address %s\n", s.Container.Container.ID, s.Container.IpAddress())
		}
	}
}
