package main

import (
	"anki/cloudproc"
	"anki/robot"
	"anki/voice"
)

func init() {
	checkDataFunc = checkCloudDataFiles
	platformOpts = append(platformOpts, cloudproc.WithVoiceOptions(voice.WithRequireToken()))
}

func checkCloudDataFiles() error {
	esn, err := robot.ReadESN()
	if err != nil {
		return err
	}

	return robot.CheckFactoryCloudFiles("/factory/cloud", esn)
}
