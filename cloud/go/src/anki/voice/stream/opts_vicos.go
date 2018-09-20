// +build vicos

package stream

import (
	"anki/log"
	"anki/robot"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

func init() {
	if esn, err := robot.ReadESN(); err != nil {
		log.Println("Couldn't read robot ESN:", err)
	} else {
		platformOpts = append(platformOpts, chipper.WithDeviceID(esn))
	}
}
