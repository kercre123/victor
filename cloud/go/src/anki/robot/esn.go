package robot

import (
	"encoding/binary"
	"fmt"
	"os"
)

// ReadESN returns the ESN of a robot by reading it from
// the robot's filesystem
func ReadESN() (string, error) {
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
