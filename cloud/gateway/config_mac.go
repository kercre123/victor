// +build darwin

package main

const (
	Port                   = 8443
	SocketPath             = "/tmp/"
	IsSwitchboardAvailable = false
)

func verifyAuthToken(_ interface{}) error {
	return nil
}
