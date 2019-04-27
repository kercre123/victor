package main

import (
	"fmt"
	"io"
	"os"
	"os/exec"
	"runtime"
	"strings"
	"sync"
	"sync/atomic"
	"syscall"

	"github.com/anki/sai-kinlog/logsender"
)

func execCommand(cmdline string, cmdargs []string, captureMode string) (shutdown func(force bool) int, r io.Reader, err error) {
	cmd := exec.Command(cmdline, cmdargs...)

	switch captureMode {
	case stderr:
		r, err = cmd.StderrPipe()
		cmd.Stdout = os.Stdout

	case stdout:
		r, err = cmd.StdoutPipe()
		cmd.Stderr = os.Stderr
	}

	if err != nil {
		return nil, nil, err
	}

	var once sync.Once
	var exitCode int32

	shutdown = func(force bool) int {
		once.Do(func() {
			if force {
				// try and shutdown cleanly if not on Windows
				if runtime.GOOS != "windows" {
					// see https://github.com/golang/go/issues/6720
					cmd.Process.Signal(os.Interrupt)
				} else {
					cmd.Process.Kill()
				}
			}

			if err := cmd.Wait(); err != nil {
				atomic.StoreInt32(&exitCode, 254)
				if exitErr, ok := err.(*exec.ExitError); ok {
					sys := exitErr.Sys()
					if status, ok := sys.(syscall.WaitStatus); ok {
						atomic.StoreInt32(&exitCode, int32(status.ExitStatus()))
					}
				}
			}
		})
		return int(atomic.LoadInt32(&exitCode))
	}

	return shutdown, r, cmd.Start()
}

type testEventWriter struct {
	closed int32
}

func (w *testEventWriter) Write(p []byte) (int, error) {
	if atomic.LoadInt32(&w.closed) > 0 {
		return 0, logsender.ErrWriterClosed
	}
	fmt.Println(strings.Repeat("-", 80))
	fmt.Printf(string(p))
	return len(p), nil
}

func (w *testEventWriter) Close() error {
	atomic.StoreInt32(&w.closed, 1)
	return nil
}
