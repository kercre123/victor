package main

import (
	"anki/cloudproc"
	"anki/config"
	"anki/jdocs"
	"anki/log"
	"anki/logcollector"
	"anki/token"
	"anki/token/jwt"
	"anki/voice"
	"clad/cloud"
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
	return []token.Option{token.WithServer()}
}

func jdocsServiceOptions(tokener token.Accessor) []jdocs.Option {
	return []jdocs.Option{
		jdocs.WithServer(),
		jdocs.WithTokener(tokener),
	}
}

func logcollectorServiceOptions(tokener token.Accessor) []logcollector.Option {
	return []logcollector.Option{
		logcollector.WithServer(),
		logcollector.WithTokener(tokener),
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
	id int

	io      voice.MsgIO
	process *voice.Process

	tokenClient        *tokenClient
	jdocsClient        *jdocsClient
	logcollectorClient *logcollectorClient
	micClient          *micClient
}

func newTestableRobot(urlConfigFile string) *testableRobot {
	testableRobot := new(testableRobot)

	if err := config.SetGlobal(urlConfigFile); err != nil {
		log.Println("Could not load server config! This is not good!:", err)
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

func (r *testableRobot) run(urlConfigFile string) {
	if err := config.SetGlobal(urlConfigFile); err != nil {
		log.Println("Could not load server config! This is not good!:", err)
	}

	path := fmt.Sprintf("%s_%04d", jwt.DefaultTokenPath, r.id)
	identityProvider := jwt.NewIdentityProvider(path)
	tokener := token.GetAccessor(identityProvider)

	options := []cloudproc.Option{cloudproc.WithIdentityProvider(identityProvider)}

	options = append(options, cloudproc.WithVoice(r.process))
	options = append(options, cloudproc.WithVoiceOptions(voiceServiceOptions(false, false)...))
	options = append(options, cloudproc.WithTokenOptions(tokenServiceOptions()...))
	options = append(options, cloudproc.WithJdocs(jdocsServiceOptions(tokener)...))
	options = append(options, cloudproc.WithLogCollectorOptions(logcollectorServiceOptions(tokener)...))

	cloudproc.Run(context.Background(), options...)
}

func (r *testableRobot) waitUntilReady() {
	// TODO: implement proper signaling to indicate that servers are accepting connections
	time.Sleep(time.Second)
}
