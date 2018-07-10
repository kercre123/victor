package cloudproc

import (
	"anki/token"
	"sync"
)

func Run(procOptions ...Option) {
	var opts options
	for _, o := range procOptions {
		o(&opts)
	}

	var wg sync.WaitGroup
	launchProcess(&wg, func() {
		token.Run(opts.tokenOpts...)
	})
	if opts.voice != nil {
		launchProcess(&wg, func() {
			opts.voice.Run(opts.voiceOpts...)
		})
	}
	wg.Wait()
}

func launchProcess(wg *sync.WaitGroup, launcher func()) {
	wg.Add(1)
	go func() {
		launcher()
		wg.Done()
	}()
}
