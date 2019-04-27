/*
Command log2kinesis accepts log data on stdin and transmits it to a
Kinesis stream.
*/
package main

import (
	"fmt"
	"os"

	"github.com/jawher/mow.cli"
)

const (
	stderr = "stderr"
	stdout = "stdout"
)

var (
	hostname          = "unknown"
	defaultSourceType = "generic"
)

func init() {
	if hn, err := os.Hostname(); err == nil {
		hostname = hn
	}
}

func usage(cmd *cli.Cmd, msg string) {
	fmt.Fprintln(os.Stderr, msg)
	cmd.PrintHelp()
	cli.Exit(1)
}

func fail(format string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, format+"\n", a...)
	cli.Exit(100)
}

func main() {
	app := cli.App("log2kinesis", "Sends log data to a Kinesis stream")

	checkCommand(app)
	sendCommand(app)

	app.Run(os.Args)
}
