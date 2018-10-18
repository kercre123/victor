package main

import (
	"anki/cloudproc"
	"anki/config"
	"anki/jdocs"
	"anki/log"
	"anki/logcollector"
	"anki/token"
	"context"
	"crypto/tls"
	"fmt"
	"net/http"
	"time"

	"github.com/gwatts/rootcerts"
)

func getHTTPClient() *http.Client {
	// Create a HTTP client with given CA cert pool so we can use https on device
	return &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{RootCAs: rootcerts.ServerCertPool()},
		},
	}
}

func formatSocketName(socketName, suffix string) string {
	return fmt.Sprintf("%s_%s", socketName, suffix)
}

func tokenServiceOptions() []token.Option {
	opts := []token.Option{token.WithServer()}
	return opts
}

func jdocsServiceOptions(tokener token.Accessor) []jdocs.Option {
	opts := []jdocs.Option{jdocs.WithServer()}
	opts = append(opts, jdocs.WithTokener(tokener))
	return opts
}

func logcollectorServiceOptions(tokener token.Accessor) []logcollector.Option {
	opts := []logcollector.Option{logcollector.WithServer()}
	opts = append(opts, logcollector.WithTokener(tokener))
	opts = append(opts, logcollector.WithHTTPClient(getHTTPClient()))
	opts = append(opts, logcollector.WithS3UrlPrefix(config.Env.LogFiles))
	opts = append(opts, logcollector.WithAwsRegion("us-west-2"))
	return opts
}

type testableRobot struct {
	tokenClient        *tokenClient
	jdocsClient        *jdocsClient
	logcollectorClient *logcollectorClient
}

func (r *testableRobot) connectClients() error {
	r.tokenClient = new(tokenClient)

	if err := r.tokenClient.connect("token_server"); err != nil {
		return err
	}

	r.jdocsClient = new(jdocsClient)
	if err := r.jdocsClient.connect("jdocs_server"); err != nil {
		return err
	}

	r.logcollectorClient = new(logcollectorClient)
	if err := r.logcollectorClient.connect("logcollector_server"); err != nil {
		return err
	}

	return nil
}

func (r *testableRobot) closeClients() {
	if r.tokenClient != nil {
		r.tokenClient.conn.Close()
	}

	if r.jdocsClient != nil {
		r.jdocsClient.conn.Close()
	}

	if r.logcollectorClient != nil {
		r.logcollectorClient.conn.Close()
	}
}

func (r *testableRobot) run(urlConfigFile string) {
	if err := config.SetGlobal(urlConfigFile); err != nil {
		log.Println("Could not load server config! This is not good!:", err)
	}

	tokener := token.GetAccessor()

	var options []cloudproc.Option

	options = append(options, cloudproc.WithTokenOptions(tokenServiceOptions()...))
	options = append(options, cloudproc.WithJdocs(jdocsServiceOptions(tokener)...))
	options = append(options, cloudproc.WithLogCollectorOptions(logcollectorServiceOptions(tokener)...))

	cloudproc.Run(context.Background(), options...)
}

func (r *testableRobot) waitUntilReady() {
	// TODO: implement proper signaling to indicate that servers are accepting connections
	time.Sleep(time.Second)
}
