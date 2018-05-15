package main

import (
	"anki/cloudproc"
	"anki/ipc"
	"anki/log"
	"bytes"
	"clad/cloud"
	"flag"
	"time"
)

var verbose bool
var checkDataFunc func() error // overwritten by cert_verify_linux.go

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

func testReader(serv ipc.Server, send cloudproc.MsgSender) {
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
	if checkDataFunc != nil {
		if err := checkDataFunc(); err != nil {
			log.Println("CLOUD DATA VERIFICATION ERROR:", err)
			log.Println("(this should not happen on any DVT3 or later robot)")
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

	// set up test channel if flags say we should
	var testRecv *cloudproc.Receiver
	if test {
		testSock, err := ipc.NewUnixgramServer(ipc.GetSocketPath("cp_test"))
		if err != nil {
			log.Println("Server create error:", err)
		}
		defer testSock.Close()

		var testSend cloudproc.MsgIO
		testSend, testRecv = cloudproc.NewMemPipe()
		go testReader(testSock, testSend)
		log.Println("Test channel created")
	}
	log.Println("Sockets successfully created")

	cloudproc.SetVerbose(verbose)
	receiver := cloudproc.NewIpcReceiver(micSock, nil)

	process := &cloudproc.Process{}
	process.AddReceiver(receiver)
	if testRecv != nil {
		process.AddTestReceiver(testRecv)
	}
	process.AddIntentWriter(&cloudproc.IPCMsgSender{Conn: aiSock})
	options := []cloudproc.Option{cloudproc.WithChunkMs(120), cloudproc.WithSaveAudio(true)}
	//options = append(options, WithCompression(true))
	if *ms {
		options = append(options, cloudproc.WithHandler(cloudproc.HandlerMicrosoft))
	} else if *lex {
		options = append(options, cloudproc.WithHandler(cloudproc.HandlerAmazon))
	}
	process.Run(options...)
}
