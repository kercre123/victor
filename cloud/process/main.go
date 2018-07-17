package main

import (
	"anki/cloudproc"
	"anki/ipc"
	"anki/log"
	"anki/token"
	"anki/voice"
	"bytes"
	"clad/cloud"
	"flag"
	"time"
)

var verbose bool
var checkDataFunc func() error // overwritten by cert_verify_linux.go
var certErrorFunc func() bool  // overwritten by cert_error_shipping.go, determines if error should cause exit
var platformOpts []cloudproc.Option

func getSocketWithRetry(name string, client string) ipc.Conn {
	for {
		sock, err := ipc.NewUnixgramClient(name, client)
		if err != nil {
			log.Println("Couldn't create socket", name, "- retrying:", err)
			time.Sleep(5 * time.Second)
		} else {
			return sock
		}
	}
}

func testReader(serv ipc.Server, send voice.MsgSender) {
	for conn := range serv.NewConns() {
		go func(conn ipc.Conn) {
			for {
				msg := conn.ReadBlock()
				if msg == nil || len(msg) == 0 {
					conn.Close()
					return
				}
				var cmsg cloud.Message
				if err := cmsg.Unpack(bytes.NewBuffer(msg)); err != nil {
					log.Println("Test reader unpack error:", err)
					continue
				}
				send.Send(&cmsg)
			}
		}(conn)
	}
}

func main() {
	log.Println("Hello, world!")

	// if we want to error, we should do it after we get socket connections, to make sure
	// vic-anim is running and able to handle it
	var tryErrorFunc bool
	if checkDataFunc != nil {
		if err := checkDataFunc(); err != nil {
			log.Println("CLOUD DATA VERIFICATION ERROR:", err)
			log.Println("(this should not happen on any DVT3 or later robot)")
			tryErrorFunc = true
		} else {
			log.Println("Cloud data verified")
		}
	}

	// don't yet have control over process startup on DVT2, set these as default
	verbose = true
	test := true

	// flag.BoolVar(&verbose, "verbose", false, "enable verbose logging")
	// var test bool
	// flag.BoolVar(&test, "test", false, "enable test channel")

	ms := flag.Bool("ms", false, "force microsoft handling on the server end")
	lex := flag.Bool("lex", false, "force amazon handling on the server end")
	flag.Parse()

	micSock := getSocketWithRetry(ipc.GetSocketPath("mic_sock"), "cp_mic")
	defer micSock.Close()
	aiSock := getSocketWithRetry(ipc.GetSocketPath("ai_sock"), "cp_ai")
	defer aiSock.Close()

	// now that we have connection, we can error if necessary
	if tryErrorFunc && certErrorFunc != nil && certErrorFunc() {
		return
	}

	// set up test channel if flags say we should
	var testRecv *voice.Receiver
	if test {
		testSock, err := ipc.NewUnixgramServer(ipc.GetSocketPath("cp_test"))
		if err != nil {
			log.Println("Server create error:", err)
		}
		defer testSock.Close()

		var testSend voice.MsgIO
		testSend, testRecv = voice.NewMemPipe()
		go testReader(testSock, testSend)
		log.Println("Test channel created")
	}
	log.Println("Sockets successfully created")

	voice.SetVerbose(verbose)
	receiver := voice.NewIpcReceiver(micSock, nil)

	process := &voice.Process{}
	process.AddReceiver(receiver)
	if testRecv != nil {
		process.AddTestReceiver(testRecv)
	}
	process.AddIntentWriter(&voice.IPCMsgSender{Conn: aiSock})
	voiceOpts := []voice.Option{voice.WithChunkMs(120), voice.WithSaveAudio(true)}
	var options []cloudproc.Option
	options = append(options, platformOpts...)
	voiceOpts = append(voiceOpts, voice.WithCompression(true))
	if *ms {
		voiceOpts = append(voiceOpts, voice.WithHandler(voice.HandlerMicrosoft))
	} else if *lex {
		voiceOpts = append(voiceOpts, voice.WithHandler(voice.HandlerAmazon))
	}

	options = append(options, cloudproc.WithVoice(process))
	options = append(options, cloudproc.WithVoiceOptions(voiceOpts...))
	tokenOpts := []token.Option{token.WithServer()}
	options = append(options, cloudproc.WithTokenOptions(tokenOpts...))

	cloudproc.Run(options...)

	log.Println("All processes exited, shutting down")
}
