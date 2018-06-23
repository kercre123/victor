// +build !shipping,linux

package main

import (
	"anki/log"
	"anki/robot"
)

func init() {
	certErrorFunc = onCertError
}

func onCertError() bool {
	if err := robot.WriteFaceErrorCode(850); err != nil {
		log.Println("Error writing error code (isn't it ironic?):", err)
	}
	return true
}
