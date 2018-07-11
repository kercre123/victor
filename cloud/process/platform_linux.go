package main

import (
	"anki/cloudproc"
	"anki/robot"
)

func init() {
	checkDataFunc = checkCloudDataFiles
	platformOpts = append(platformOpts, cloudproc.WithRequireToken())
}

func checkCloudDataFiles() error {
	esn, err := robot.ReadESN()
	if err != nil {
		return err
	}

	return robot.CheckFactoryCloudFiles("/factory/cloud", esn)
}
