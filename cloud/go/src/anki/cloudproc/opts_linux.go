package cloudproc

import (
	"os/exec"
	"strings"

	"github.com/anki/sai-chipper-voice/client/chipper"
	"google.golang.org/grpc"
)

func init() {
	var agent string

	runCmd := func(cmd string, params ...string) bool {
		buf, err := exec.Command(cmd, params...).Output()
		if err != nil {
			return false
		}
		str := strings.TrimSpace(string(buf))
		if len(str) > 0 {
			agent = "Victor/" + str
			return true
		}
		return false
	}

	propArgs := []string{"ro.build.fingerprint", "ro.revision",
		"ro.anki.os_build_comment", "ro.build.version.release"}

	for _, arg := range propArgs {
		if runCmd("getprop", arg) {
			break
		}
	}

	if agent != "" {
		platformOpts = append(platformOpts, chipper.WithGrpcOptions(grpc.WithUserAgent(agent)))
	}
}
