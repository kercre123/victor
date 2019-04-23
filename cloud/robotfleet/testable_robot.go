package main

import (
	"anki/cloudproc"
	"anki/config"
	"anki/jdocs"
	"anki/log"
	"anki/logcollector"
	"anki/token"
	"anki/token/identity"
	"anki/voice"
	"clad/cloud"
	"context"
	"crypto/tls"
	"fmt"
	"net/http"
	"strconv"
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

func formatSocketName(socketName string, id int) string {
	return fmt.Sprintf("%s_%d", socketName, id)
}

func tokenServiceOptions(socketNameSuffix string) []token.Option {
	return []token.Option{
		token.WithServer(),
		token.WithSocketNameSuffix(socketNameSuffix),
	}
}

func jdocsServiceOptions(socketNameSuffix string) []jdocs.Option {
	return []jdocs.Option{
		jdocs.WithServer(),
		jdocs.WithSocketNameSuffix(socketNameSuffix),
	}
}

func logcollectorServiceOptions(socketNameSuffix string) []logcollector.Option {
	return []logcollector.Option{
		logcollector.WithServer(),
		logcollector.WithSocketNameSuffix(socketNameSuffix),
		logcollector.WithHTTPClient(getHTTPClient()),
		logcollector.WithS3UrlPrefix(config.Env.LogFiles),
		logcollector.WithAwsRegion("us-west-2"),
	}
}

func voiceServiceOptions(ms, lex bool) []voice.Option {
	opts := []voice.Option{
		voice.WithChunkMs(120),
		voice.WithSaveAudio(true),
		voice.WithCompression(true),
	}
	if ms {
		opts = append(opts, voice.WithHandler(voice.HandlerMicrosoft))
	} else if lex {
		opts = append(opts, voice.WithHandler(voice.HandlerAmazon))
	}
	return opts
}

type testableRobot struct {
	options         *options
	instanceOptions *instanceOptions

	io      voice.MsgIO
	process *voice.Process

	tokenClient        *tokenClient
	jdocsClient        *jdocsClient
	logcollectorClient *logcollectorClient
	micClient          *micClient
}

func newTestableRobot(options *options, instanceOptions *instanceOptions) *testableRobot {
	testableRobot := &testableRobot{
		options:         options,
		instanceOptions: instanceOptions,
	}

	voice.SetVerbose(true)

	intentResult := make(chan *cloud.Message)

	var receiver *voice.Receiver
	testableRobot.io, receiver = voice.NewMemPipe()

	testableRobot.process = &voice.Process{}
	testableRobot.process.AddReceiver(receiver)
	testableRobot.process.AddIntentWriter(&voice.ChanMsgSender{Ch: intentResult})

	return testableRobot
}

func (r *testableRobot) connectClients() error {
	r.tokenClient = new(tokenClient)

	id := r.instanceOptions.robotID
	if err := r.tokenClient.connect(formatSocketName("token_server", id)); err != nil {
		return err
	}

	r.jdocsClient = new(jdocsClient)
	if err := r.jdocsClient.connect(formatSocketName("jdocs_server", id)); err != nil {
		return err
	}

	r.logcollectorClient = new(logcollectorClient)
	if err := r.logcollectorClient.connect(formatSocketName("logcollector_server", id)); err != nil {
		return err
	}

	r.micClient = &micClient{r.io}

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

func (r *testableRobot) run() {
	jwtPath := fmt.Sprintf("%s_%d", identity.DefaultTokenPath, r.instanceOptions.robotID)
	identityProvider, err := identity.NewFileProvider(jwtPath, r.instanceOptions.cloudDir)
	if err != nil {
		log.Println("Error: could not create identity provider")
		return
	}

	options := []cloudproc.Option{cloudproc.WithIdentityProvider(identityProvider)}

	options = append(options, cloudproc.WithVoice(r.process))
	options = append(options, cloudproc.WithVoiceOptions(voiceServiceOptions(false, false)...))

	socketNameSuffix := strconv.Itoa(r.instanceOptions.robotID)
	options = append(options, cloudproc.WithTokenOptions(tokenServiceOptions(socketNameSuffix)...))
	options = append(options, cloudproc.WithJdocs(jdocsServiceOptions(socketNameSuffix)...))
	options = append(options, cloudproc.WithLogCollectorOptions(logcollectorServiceOptions(socketNameSuffix)...))

	cloudproc.Run(context.Background(), options...)
}

func (r *testableRobot) waitUntilReady() {
	// TODO: implement proper signaling to indicate that servers are accepting connections
	time.Sleep(time.Second)
}
