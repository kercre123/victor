package robot

import (
	"anki/log"
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"strings"

	"google.golang.org/grpc"
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

var osVersion string

// OSVersion returns a string representation of the OS version, like:
// v0.10.1252d_os0.10.1252d-79470cd-201806271633
func OSVersion() string {
	if osVersion != "" {
		return osVersion
	}
	runCmd := func(cmd string, params ...string) string {
		buf, err := exec.Command(cmd, params...).Output()
		if err != nil {
			return ""
		}
		return strings.TrimSpace(string(buf))
	}

	propArgs := []string{"ro.build.fingerprint", "ro.revision",
		"ro.anki.os_build_comment", "ro.build.version.release"}

	for _, arg := range propArgs {
		if str := runCmd("getprop", arg); str != "" {
			osVersion = str
			return osVersion
		}
	}
	return ""
}

// OSUserAgent returns a grpc.DialOption that will set a user agent with the
// string: "Victor/<os_version>", if the OS version can be obtained. Otherwise,
// nil is returned.
func OSUserAgent() grpc.DialOption {
	ver := OSVersion()
	if ver == "" {
		return nil
	}
	return grpc.WithUserAgent("Victor/" + ver)
}

var bootID string

// BootID returns the unique ID generated on each robot bootup
func BootID() string {
	if bootID == "" {
		if buf, err := ioutil.ReadFile("/proc/sys/kernel/random/boot_id"); err != nil {
			log.IfVicos("Error reading boot ID:", err)
		} else {
			bootID = string(buf)
		}
	}
	return bootID
}
