package main

import (
	"bytes"
	"context"
	"encoding/binary"
	"fmt"
	"log"

	"flag"
	"github.com/anki/sai-chipper-voice/client/chipper"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/gordonklaus/portaudio"
	"github.com/kr/pretty"
)

const (
	inputChannels = 1
	sampleRate    = 16000
	bufSize       = sampleRate * 2 * .120 // 100ms
)

var (
	serverURL = flag.String("url", "", "The server address in the format of host:port")
	svc       = flag.String("service", "goog", "intent-service, goog [default]/ms")
	compress  = flag.Bool("compress", false, "use opus compression, true [default]/false")
	asr       = flag.Bool("asr", false, "use chipper for speech only, no intent-matching")
	kg        = flag.Bool("kg", false, "send a kg request instead of an intent request")
	cc        = flag.Bool("cc", false, "send a connection check instead of an intent request")
	insecure  = flag.Bool("insecure", false, "send requests without TLS")
)

func main() {

	flag.Parse()

	service := pb.IntentService_DIALOGFLOW
	if *svc == "ms" {
		service = pb.IntentService_BING_LUIS
	}

	// init portaudio
	portaudio.Initialize()
	defer portaudio.Terminate()
	fmt.Println("Initialized portaudio")

	conn, err := chipper.NewConn(context.Background(), *serverURL, chipper.WithDeviceID("micsend"), chipper.WithInsecure(*insecure), chipper.WithSessionID("session-micsend"))
	if err != nil {
		log.Fatal("failed to create client")
	}
	defer conn.Close()
	fmt.Println("Initialized Chipper connection", *serverURL)

	var stream chipper.Stream
	if *kg {
		opts := chipper.StreamOpts{
			CompressOpts: chipper.CompressOpts{
				Compress:   *compress,
				Bitrate:    66 * 1024,
				Complexity: 0,
				FrameSize:  60},
		}
		stream, err = conn.NewKGStream(context.Background(), opts)
		if err != nil {
			log.Fatal("failed to create stream")
		}
	} else if *cc {
		opts := chipper.ConnectOpts{
			StreamOpts: chipper.StreamOpts{
				CompressOpts: chipper.CompressOpts{
					Compress:   *compress,
					Bitrate:    66 * 1024,
					Complexity: 0,
					FrameSize:  60},
			},
			TotalAudioMs:      6000,
			AudioPerRequestMs: 120,
		}
		stream, err = conn.NewConnectionStream(context.Background(), opts)
		if err != nil {
			log.Fatal("failed to create stream")
		}
	} else {
		opts := chipper.IntentOpts{
			Handler:    service,
			Mode:       pb.RobotMode_VOICE_COMMAND,
			SpeechOnly: *asr,
			StreamOpts: chipper.StreamOpts{
				CompressOpts: chipper.CompressOpts{
					Compress:   *compress,
					Bitrate:    66 * 1024,
					Complexity: 0,
					FrameSize:  60},
			},
		}

		stream, err = conn.NewIntentStream(context.Background(), opts)

		if err != nil {
			log.Fatal("failed to create stream")
		}
	}
	defer stream.Close()
	fmt.Println("Initialized Chipper stream")

	audioStream := make(chan []byte, 10)
	notice := make(chan struct{})

	go sender(stream, audioStream, notice)
	go readAudio(audioStream)

	// wait for the first audio frame to be transmitted
	<-notice
	intent, err := stream.WaitForResponse()
	if err != nil {
		log.Fatal("Intent read failed", err)
	}
	pretty.Println(intent)
}

func readAudio(audioStream chan<- []byte) {
	//buf := make([]byte, sampleRate*2)
	buf := make([]int16, sampleRate*0.120)
	outbuf := new(bytes.Buffer)

	flush := func() {
		audioStream <- outbuf.Bytes()
		outbuf = new(bytes.Buffer)
	}

	stream, err := portaudio.OpenDefaultStream(inputChannels, 0, float64(sampleRate), len(buf), buf)
	if err != nil {
		log.Fatal("stream open failed", err)
	}
	defer stream.Close()

	if err := stream.Start(); err != nil {
		log.Fatal("Failed to start stream", err)
	}

	/*
		outfile, err := os.Create("/tmp/out.wav")
		if err != nil {
			log.Fatal("write open failed", err)
		}
	*/

	for {
		for outbuf.Len() < bufSize {
			if err := stream.Read(); err != nil {
				if err != nil {
					log.Fatal("stream read failed", err)
				}
			}
			binary.Write(outbuf, binary.LittleEndian, buf)
		}
		flush()
	}
}

func sender(client chipper.Stream, audioStream <-chan []byte, notice chan<- struct{}) {
	first := true
	for data := range audioStream {
		if err := client.SendAudio(data); err != nil {
			log.Fatal("Audio send failed: ", err)
		}
		if first {
			close(notice)
			first = false
		}

		fmt.Println("Audio frame sent ok")
	}
}
