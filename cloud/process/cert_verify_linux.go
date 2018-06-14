package main

import (
	"anki/robot"
	"encoding/binary"
	"fmt"
	"os"
)

func init() {
	checkDataFunc = checkCloudDataFiles
}

// TODO: put this functionality in a centralized location so there aren't
// 3 places in our code base hardcoding the location of the EMR file
func readESN() (string, error) {
	file, err := os.Open("/dev/block/bootdevice/by-name/emr")
	if err != nil {
		return "", err
	}
	defer file.Close()

	// ESN is first 4 bytes of the file
	var numEsn uint32
	if err := binary.Read(file, binary.LittleEndian, &numEsn); err != nil {
		return "", err
	}
	return fmt.Sprintf("%08x", numEsn), nil
}

func checkCloudDataFiles() error {
	esn, err := readESN()
	if err != nil {
		return err
	}

	return robot.CheckFactoryCloudFiles("/factory/cloud", esn)
}
