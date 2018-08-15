package cloudproc

import (
	"anki/jdocs"
	"anki/token"
	"anki/voice"
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
	tokener := token.GetAccessor()
	if opts.voice != nil {
		launchProcess(&wg, func() {
			// provide default token accessor
			voiceOpts := append([]voice.Option{voice.WithTokener(tokener)},
				opts.voiceOpts...)
			opts.voice.Run(ctx, voiceOpts...)
		})
	}
	if opts.jdocOpts != nil {
		launchProcess(&wg, func() {
			// provide default token accessor
			jdocOpts := append([]jdocs.Option{jdocs.WithTokener(tokener)},
				opts.jdocOpts...)
			jdocs.Run(ctx, jdocOpts...)
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
