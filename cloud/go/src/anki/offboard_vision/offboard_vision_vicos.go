// +build !shipping,vicos

package offboard_vision

import (
	"anki/log"
	"anki/robot"
)

func init() {
	if esn, err := robot.ReadESN(); err != nil {
		log.Println("Couldn't read robot ESN:", err)
	} else {
		deviceID = esn
	}
}
