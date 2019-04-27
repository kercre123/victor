package svc

import (
	"context"
	"encoding/json"
	"net/http"
	"sync"

	"github.com/gwatts/manners"
	"github.com/jawher/mow.cli"

	"github.com/anki/sai-go-util/buildinfo"
	"github.com/anki/sai-go-util/http/metrics/profileserver"
	"github.com/anki/sai-go-util/log"
)

// ProfileServerComponent is a service object component which launches
// an HTTP server exposing the Golang expvar package, to allow
// collection of metrics, runtime stats, and other Go runtime
// information.
type ProfileServerComponent struct {
	port   *int
	server *manners.GracefulServer
	wg     sync.WaitGroup
}

func (c *ProfileServerComponent) CommandSpec() string {
	return "[--profile-port]"
}

func (c *ProfileServerComponent) CommandInitialize(cmd *cli.Cmd) {
	c.port = IntOpt(cmd, "profile-port", 0,
		"If set then an HTTP profile server will be available on this port number")
}

func (c *ProfileServerComponent) Start(ctx context.Context) error {
	// Start profile server for metrics collection
	if *c.port > 0 {
		alog.Info{
			"action": "profile_server",
			"status": alog.StatusStart,
			"port":   *c.port,
		}.Log()
		errChan := make(chan error)
		c.server = profileserver.NewServer(*c.port)
		go func() {
			c.wg.Add(1)
			go func() {
				errChan <- c.server.ListenAndServe()
			}()
			err := <-errChan
			if err != nil {
				alog.Error{
					"action": "profile_server",
					"status": alog.StatusError,
					"error":  err,
				}.Log()
			} else {
				alog.Info{
					"action": "profile_server",
					"status": alog.StatusDone,
				}.Log()
			}
			c.wg.Done()
		}()
	}

	return nil
}

func (c *ProfileServerComponent) Stop() error {
	if c.server != nil {
		c.server.Close()
		c.wg.Wait()
	}
	return nil
}

func (c *ProfileServerComponent) Kill() error {
	if c.server != nil {
		// Close, but don't wait
		c.server.Close()
	}
	return nil
}

func WithProfileServer() ConfigFn {
	return WithComponent(&ProfileServerComponent{})
}

func init() {
	// Also add some useful endpoints on top of the main ones already
	// exposed by the expvar package.

	// Write out the build info as JSON
	http.Handle("/version", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		js, _ := json.Marshal(buildinfo.Info())
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		w.Write(js)
	}))

	// Add a root level handler that hyperlinks to the other debug
	// endpoints that are available
	http.Handle("/", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html")
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(`
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Profile Server</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/bulma/0.6.2/css/bulma.min.css">
  </head>
  <body>
  <section class="section">
    <div class="container">
<div class="content">
<ul>
<li><a href="/version">Build Version</a></li>
<li><a href="/status">Status Checks</a></li>
<li><a href="/debug/pprof">Profiling</a></li>
<li><a href="/debug/metrics">Metrics</a></li>
<li><a href="/debug/metrics/charts">Metrics Charts</a></li>
</ul>
    </div>
    </div>
  </section>
  </body>
</html>
<html><body>
</body></html>
`))
	}))
}
