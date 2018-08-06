package cloudproc

import (
	"anki/token"
	"context"
	"log"
	"sync"
)

var devServer func() error

func Run(ctx context.Context, procOptions ...Option) {
	var opts options
	for _, o := range procOptions {
		o(&opts)
	}

	var wg sync.WaitGroup
	if devServer != nil {
		launchProcess(&wg, func() {
			if err := devServer(); err != nil {
				log.Println("dev HTTP server reported error:", err)
			}
		})
	}
	launchProcess(&wg, func() {
		addHandlers(token.GetDevHandlers)
		token.Run(ctx, opts.tokenOpts...)
	})
	if opts.voice != nil {
		launchProcess(&wg, func() {
			opts.voice.Run(ctx, opts.voiceOpts...)
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
