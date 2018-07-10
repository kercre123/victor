package voice

import (
	"anki/log"
	"anki/robot"

	"github.com/anki/sai-chipper-voice/client/chipper"
)

func init() {
	if opt := robot.OSUserAgent(); opt != nil {
		platformOpts = append(platformOpts, chipper.WithGrpcOptions(opt))
	}
	if esn, err := robot.ReadESN(); err != nil {
		log.Println("Couldn't read robot ESN:", err)
	} else {
		platformOpts = append(platformOpts, chipper.WithDeviceID(esn))
	}
}
