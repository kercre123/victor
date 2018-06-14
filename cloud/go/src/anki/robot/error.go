package robot

import (
	"encoding/binary"
	"os"
)

// WriteFaceErrorCode writes the given numerical code to the robot's face with an error message
func WriteFaceErrorCode(code uint16) error {
	file, err := os.OpenFile("/run/error_code", os.O_RDWR|os.O_APPEND, 0777)
	if err != nil {
		return err
	}
	defer file.Close()
	return binary.Write(file, binary.LittleEndian, code)
}
