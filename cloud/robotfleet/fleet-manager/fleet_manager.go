package main

import (
	"errors"
	"fmt"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/ecs"
	"github.com/go-redis/redis"
)

// see distributedController.go
const (
	taskIDKey            = "task_id"
	remoteControlChannel = "load_test_control"
)

type fleetManager struct {
	clusterName string
	ecsSvc      *ecs.ECS
	redisClient *redis.Client
}

func newFleetManager(redisAddress, clusterName, awsRegion string) *fleetManager {
	return &fleetManager{
		clusterName: clusterName,
		redisClient: redis.NewClient(&redis.Options{Addr: redisAddress}),
		ecsSvc:      ecs.New(session.Must(session.NewSession()), &aws.Config{Region: aws.String(awsRegion)}),
	}
}

func (m *fleetManager) resetTaskID() error {
	return m.redisClient.Set(taskIDKey, "0", 0).Err()
}

func (m *fleetManager) fleetTaskID() (string, error) {
	return m.redisClient.Get(taskIDKey).Result()
}

func (m *fleetManager) commandFleet(command string) (int64, error) {
	return m.redisClient.Publish(remoteControlChannel, command).Result()
}

func (m *fleetManager) updateService() error {
	// This "UpdateService" call forces a new deployment of the container
	// image specified in the task definition, allowing us to redeploy
	// without creating a new revision of the task definition.
	// For now this assumes using a single service deployment (index 0)
	_, err := m.ecsSvc.UpdateService(&ecs.UpdateServiceInput{
		Cluster: aws.String(m.clusterName),
		Service: aws.String(fmt.Sprintf("%s_0", m.clusterName)),
		DeploymentConfiguration: &ecs.DeploymentConfiguration{
			MaximumPercent:        aws.Int64(200),
			MinimumHealthyPercent: aws.Int64(0),
		},
		ForceNewDeployment: aws.Bool(true),
	})

	return err
}

func (m *fleetManager) primaryDeploymentAvailable() (bool, error) {
	// Retrieve last assigned task ID
	taskID, err := m.fleetTaskID()
	if err != nil && err != redis.Nil {
		return false, err
	}

	describeServicesOutput, err := m.ecsSvc.DescribeServices(&ecs.DescribeServicesInput{
		Cluster:  aws.String(m.clusterName),
		Services: []*string{aws.String(m.clusterName + "_0")},
	})

	if err != nil {
		return false, err
	}

	// We expect only a single server to retun since we query only one (by name)
	if len(describeServicesOutput.Services) != 1 {
		return false, errors.New("no services found")
	}

	// Get primary deployment (this is the one most recently updated)
	var primaryDeployment *ecs.Deployment
	for _, deployment := range describeServicesOutput.Services[0].Deployments {
		if *deployment.Status == "PRIMARY" {
			primaryDeployment = deployment
		}
	}

	fmt.Printf("Deployment progress: %d of %d tasks running (taskID=%q)\n", *primaryDeployment.RunningCount, *primaryDeployment.DesiredCount, taskID)

	return *primaryDeployment.RunningCount == *primaryDeployment.DesiredCount, nil
}

func (m *fleetManager) waitForRobotFleet(timeoutDuration, intervalDuration time.Duration) error {
	fmt.Printf("Checking robot fleet deployment (polling interval of %v and timeout of %v)\n", intervalDuration, timeoutDuration)

	ticker := time.NewTicker(intervalDuration)
	defer ticker.Stop()

	timeout := time.After(timeoutDuration)

	for {
		select {
		case <-timeout:
			return errors.New("deployment wait timeout")
		case <-ticker.C:
			available, err := m.primaryDeploymentAvailable()
			if err != nil {
				return err
			}

			if available {
				return nil
			}
		}
	}
}

func (m *fleetManager) deployRobotFleet(timeout, interval time.Duration) {
	fmt.Println("Starting deployment...")

	// We first reset the task ID assignment counter to zero in order to
	// ensure that new containers get task IDs assigned starting from 0
	err := m.resetTaskID()
	if err != nil {
		fmt.Println(`Fleet "reset" error: %v`, err)
		return
	}
	fmt.Println("Task ID assignment counter reset to zero")

	err = m.updateService()
	if err != nil {
		fmt.Println("Deployment (update service) error:", err)
		return
	}

	err = m.waitForRobotFleet(timeout, interval)
	if err != nil {
		fmt.Println("Deployment (waiting for fleet) error:", err)
		return
	}

	fmt.Println("Deployment finished...")
}

func (m *fleetManager) controlRobotFleet(command string) {
	numTasks, err := m.commandFleet(command)
	if err != nil {
		fmt.Printf("Fleet %q error: %v\n", command, err)
		return
	}

	fmt.Printf("Fleet control command %q received by %d tasks\n", command, numTasks)
}
