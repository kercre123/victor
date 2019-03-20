// +build vicos

package main

import (
	"anki/robot"
)

func init() {
	checkDataFunc = checkCloudDataFiles
}

func checkCloudDataFiles() error {
	esn, err := robot.ReadESN()
	if err != nil {
		return err
	}

	return robot.CheckFactoryCloudFiles(robot.DefaultCloudDir, esn)
}
