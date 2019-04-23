package main

import (
	"errors"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
)

type fakeIDProvider struct {
	taskID      int
	returnError bool
}

func (p *fakeIDProvider) uniqueTaskID() (int, error) {
	if p.returnError {
		return 0, errors.New("could not obtain task id")
	}

	p.taskID++
	return p.taskID, nil
}

// Creates a monotonically increasing fake arrival time as a function of the robotID, used to
// verify ramping strategies.
func (p *fakeIDProvider) arrivalTime(robotID int) (time.Duration, error) {
	return time.Duration(robotID) * time.Second, nil
}

// Creates a monotonically increasing fake departure time as a function of the robotID, used to
// verify ramping strategies.
func (p *fakeIDProvider) departureTime(robotID int) (time.Duration, error) {
	return time.Duration(robotID) * 10 * time.Second, nil
}

type OptionsTestSuite struct {
	suite.Suite
}

func createTestOptions(numberOfCerts, robotsPerProcess, tasksPerCluster int, defaultTestUserName, defaultCloudDir string) options {
	return options{
		numberOfCerts:       &numberOfCerts,
		robotsPerProcess:    &robotsPerProcess,
		tasksPerCluster:     &tasksPerCluster,
		robotsPerCluster:    robotsPerProcess * tasksPerCluster,
		defaultTestUserName: &defaultTestUserName,
		defaultCloudDir:     &defaultCloudDir,
	}
}

func (s *OptionsTestSuite) TestIDProvisioningFirstTask() {
	robotIndex := 4
	defaultTestUserName := "default-test-user-name"
	defaultCloudDir := "default-cloud-dir"

	options := createTestOptions(100, 10, 2, defaultTestUserName, defaultCloudDir)
	provider := &fakeIDProvider{}
	instanceOptions := options.createIdentity(provider, robotIndex, 0)

	s.Equal(8, instanceOptions.robotID)
	s.Equal(defaultTestUserName, instanceOptions.testUserName)
	s.Equal(defaultCloudDir, instanceOptions.cloudDir)
}

func (s *OptionsTestSuite) TestIDProvisioningSecondTask() {
	robotIndex := 5
	taskID := 1

	options := createTestOptions(100, 10, 2, "", "")
	provider := &fakeIDProvider{taskID: taskID}
	instanceOptions := options.createIdentity(provider, robotIndex, taskID)

	s.Equal(11, instanceOptions.robotID)
	s.Equal("test.00000011@anki.com", instanceOptions.testUserName)
	s.Equal("/device_certs/00000011", instanceOptions.cloudDir)
}

func (s *OptionsTestSuite) TestCertWrapping() {
	const robotsPerProcess = 5
	const numberOfCerts = 3

	options := createTestOptions(numberOfCerts, robotsPerProcess, 2, "default-test-user-name", "")

	provider := &fakeIDProvider{}

	expectedRobotIDs := []int{0, 2, 4, 6, 8}

	for robotIndex := 0; robotIndex < robotsPerProcess; robotIndex++ {
		instanceOption := options.createIdentity(provider, robotIndex, 0)

		// make sure IDs are interleaved across containers to distribute ramp load
		s.Equal(expectedRobotIDs[robotIndex]%numberOfCerts, instanceOption.robotID)

		// make sure cert directories wrap around to ensure directory exists
		s.Equal(fmt.Sprintf("/device_certs/%08d", expectedRobotIDs[robotIndex]%numberOfCerts), instanceOption.cloudDir)
	}
}

func (s *OptionsTestSuite) TestRampingRedis() {
	const robotsPerProcess = 2
	const tasksPerCluster = 2

	options := createTestOptions(10, robotsPerProcess, tasksPerCluster, "", "")
	s.Equal(robotsPerProcess*tasksPerCluster, options.robotsPerCluster)

	provider := &fakeIDProvider{}

	expectedRobotIDs := []int{0, 2, 1, 3}

	for taskID := 0; taskID < tasksPerCluster; taskID++ {
		for robotIndex := 0; robotIndex < robotsPerProcess; robotIndex++ {
			index := (taskID * robotsPerProcess) + robotIndex

			instanceOptions := options.createIdentity(provider, robotIndex, taskID)

			s.Equal(expectedRobotIDs[index], instanceOptions.robotID)

			expectedRampupDelay := time.Duration(expectedRobotIDs[index]) * time.Second
			s.Equal(expectedRampupDelay, instanceOptions.rampupDelay)

			expectedRampdownDelay := time.Duration(10*expectedRobotIDs[index]) * time.Second
			s.Equal(expectedRampdownDelay, instanceOptions.rampdownDelay)
		}
	}
}

func (s *OptionsTestSuite) TestRampingParams() {
	const robotsPerProcess = 3
	const tasksPerCluster = 4

	options := createTestOptions(robotsPerProcess*tasksPerCluster, robotsPerProcess, tasksPerCluster, "", "")
	s.Equal(robotsPerProcess*tasksPerCluster, options.robotsPerCluster)

	options.rampupDuration = time.Second * 12
	options.rampdownDuration = time.Second * 24

	// We expect to see the different containers (rows) have interleaved start delays
	expectedDelays := [][]int{
		{0, 4, 8},
		{1, 5, 9},
		{2, 6, 10},
		{3, 7, 11},
	}

	for taskID := 0; taskID < tasksPerCluster; taskID++ {
		for robotIndex := 0; robotIndex < robotsPerProcess; robotIndex++ {
			instanceOptions := options.createIdentity(nil, robotIndex, taskID)

			expectedRampupDelay := time.Duration(expectedDelays[taskID][robotIndex]) * time.Second
			s.Equal(expectedRampupDelay, instanceOptions.rampupDelay)

			expectedRampdownDelay := time.Duration(2*expectedDelays[taskID][robotIndex]) * time.Second
			s.Equal(expectedRampdownDelay, instanceOptions.rampdownDelay)
		}
	}
}

func (s *OptionsTestSuite) TestNoRampingParams() {
	const robotsPerProcess = 3
	const tasksPerCluster = 2

	options := createTestOptions(10, robotsPerProcess, tasksPerCluster, "", "")
	s.Equal(robotsPerProcess*tasksPerCluster, options.robotsPerCluster)

	options.rampupDuration = 0

	for taskID := 0; taskID < tasksPerCluster; taskID++ {
		for i := 0; i < robotsPerProcess; i++ {
			robotID := (taskID * robotsPerProcess) + i
			instanceOptions := options.createIdentity(nil, robotID, taskID)

			s.Equal(time.Duration(0), instanceOptions.rampupDelay)
			s.Equal(time.Duration(0), instanceOptions.rampdownDelay)
		}
	}
}

func TestOptions(t *testing.T) {
	suite.Run(t, new(OptionsTestSuite))
}
