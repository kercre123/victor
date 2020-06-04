// Copyright 2016 Anki, Inc.
// Author: Gareth Watts <gareth@anki.com>

package apirouter

import (
	"errors"
	"net/http"
	"strings"

	"github.com/gorilla/context"
)

const (
	// Only recognize agent strings beginning with "overdrive" or "cozmo" as
	// device agents.
	overdriveProductName  = "overdrive"
	foxtrotProductName    = "foxtrot"
	cozmoProductName      = "cozmo"
	deviceAgentContextKey = "device-agent"
)

// DeviceAgent holds a parsed version of a device agent string (eg. that
// sent by the Overdrive app)
type DeviceAgent struct {
	Product        string // "overdrive"
	BuildVersion   string // "1.3.0.2503.160503.1646.d.2c39eb2"
	Version        string // "1.3"
	DeviceOS       string // "ios"
	DeviceTypeName string // "iPod touch 7.1.1:
}

var padding = [4]string{"0000", "000", "00", "0"}

// NormalizeVersion converts the version string into something
// that will sort correctly when compared with other version strings.
//
// Essentially this assumes that versions are in at most four parts
// "1.2.3.4" and expands each subversion to a 4 digit number.
// returning "0001.0002.0003.0004" in this case.
func (da DeviceAgent) NormalizeVersion() string {
	result := []string{"0000", "0000", "0000", "0000"}
	parts := strings.Split(da.Version, ".")
	pc := len(parts)
	if pc > 4 {
		pc = 4
	}
	for i := 0; i < pc; i++ {
		num := parts[i]
		if len(num) < 4 {
			result[i] = padding[len(num)] + num
		} else {
			result[i] = num
		}
	}
	return strings.Join(result, ".")
}

func (da DeviceAgent) VersionGTE(other DeviceAgent) (bool, error) {
	if da.Product != other.Product {
		return false, errors.New("product names do not match")
	}
	return da.NormalizeVersion() >= other.NormalizeVersion(), nil
}

// ParseDeviceAgent parses the HTTP agent supplied by a device
// eg. overdrive G.0.0.2503.160503.1646.d.2c39eb2 G.0 ios iPod touch 7.1.1
//
// given an http.Request, r, it can be called as:
//
//   ParseDeviceAgent(r.Header.Get("User-Agent"))
func ParseDeviceAgent(agent string) (result DeviceAgent, err error) {
	parts := strings.SplitN(agent, " ", 5)
	if len(parts) != 5 {
		return result, errors.New("Not a device agent")
	}
	if parts[0] != overdriveProductName && parts[0] != cozmoProductName && parts[0] != foxtrotProductName {
		return result, errors.New("Not a device agent")
	}

	return DeviceAgent{
		Product:        parts[0],
		BuildVersion:   parts[1],
		Version:        parts[2],
		DeviceOS:       parts[3],
		DeviceTypeName: parts[4],
	}, nil
}

func DeviceAgentForRequest(r *http.Request) (agent *DeviceAgent) {
	if agent, ok := context.Get(r, deviceAgentContextKey).(*DeviceAgent); ok {
		return agent
	}

	resp, err := ParseDeviceAgent(r.Header.Get("User-Agent"))
	if err == nil {
		agent = &resp
	}
	// cache a nil entry if there is no agent in the user agent string
	context.Set(r, deviceAgentContextKey, agent)
	return agent
}
