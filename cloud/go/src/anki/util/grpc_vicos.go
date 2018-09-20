// +build vicos

package util

import (
	"anki/robot"
)

func init() {
	if opt := robot.OSUserAgent(); opt != nil {
		platformOpts = append(platformOpts, opt)
	}
}
