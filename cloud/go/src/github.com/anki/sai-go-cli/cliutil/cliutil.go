package cliutil

import (
	"fmt"
	"github.com/jawher/mow.cli"
	"os"
)

func Fail(format string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, format+"\n", a...)
	cli.Exit(100)
}
