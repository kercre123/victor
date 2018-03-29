package main

import (
	"anki/cloudproc"
	"flag"
	"fmt"
	"io/ioutil"

	"github.com/anki/sai-chipper-voice/client/chipper"
	pb "github.com/anki/sai-chipper-voice/grpc/pb"
	"github.com/google/uuid"
)

func main() {
	fname := flag.String("file", "", "filename to open")
	ms := flag.Bool("ms", false, "use ms")
	flag.Parse()

	fmt.Println("Connecting")
	conn, err := chipper.NewConn(cloudproc.ChipperURL, cloudproc.ChipperSecret)
	if err != nil {
		fmt.Println("Error starting chipper:", err)
		return
	}
	defer conn.Close()
	fmt.Println("Streaming")
	opts := chipper.StreamOpts{
		CompressOpts: chipper.CompressOpts{
			PreEncoded: true},
		SessionId: uuid.New().String()[:16]}
	if *ms {
		opts.Handler = pb.IntentService_BING_LUIS
	}
	stream, err := conn.NewStream(opts)
	if err != nil {
		fmt.Println("Error creating stream:", err)
		return
	}
	defer stream.Close()

	fmt.Println("Reading file")
	data, err := ioutil.ReadFile(*fname)
	if err != nil {
		fmt.Println("Error reading file", *fname, err)
		return
	}
	fmt.Println("Read file with size", len(data))

	for len(data) > 0 {
		size := 1024
		if len(data) < size {
			size = len(data)
		}
		chunk := data[:size]
		data = data[size:]
		err = stream.SendAudio(chunk)
		if err != nil {
			fmt.Println("Send error:", err)
			return
		}
	}

	fmt.Println("Waiting for intent...")
	result, err := stream.WaitForIntent()
	if err != nil {
		fmt.Println("Intent error", err)
		return
	}

	fmt.Println("Result action:", result.Action)
	fmt.Println("Result\n", result)
}
