// +build vicos

package main

import (
	"anki/robot"
)

func init() {
	checkDataFunc = checkCloudDataFiles
	// TODO: this is only for thebox, should not be done on master
	//platformOpts = append(platformOpts, cloudproc.WithVoiceOptions(voice.WithRequireToken()))
}

func checkCloudDataFiles() error {
	esn, err := robot.ReadESN()
	if err != nil {
		return err
	}

	return robot.CheckFactoryCloudFiles(robot.DefaultCloudDir, esn)
}
