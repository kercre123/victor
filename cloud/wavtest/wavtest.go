package main

import (
	"anki/ipc"
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"
	"sync"
	"time"

	"anki/cloudproc"
	"anki/cloudproc/harness"

	wav "github.com/youpy/go-wav"
)

func main() {
	infile := flag.String("file", "", "wav file to open")
	makeproc := flag.Bool("harness", false, "create cloud process harness inside this")
	verbose := flag.Bool("verbose", false, "enable verbose logging")
	compress := flag.Bool("compress", false, "do compression in harness")
	ms := flag.Bool("ms", false, "use microsoft instead of google")
	flag.Parse()

	if *infile == "" {
		fmt.Println("Error: need a filename on the command line")
		return
	}
	f, err := os.Open(*infile)
	if err != nil {
		fmt.Println("Couldn't open file", *infile, ":", err)
		return
	}

	reader := wav.NewReader(f)
	format, err := reader.Format()
	if err != nil {
		fmt.Println("Error getting format:", err)
		return
	}
	if format.BitsPerSample != cloudproc.SampleBits || format.NumChannels != 1 || format.SampleRate != cloudproc.SampleRate {
		fmt.Println("Unexpected format, expected 16 bits, 1 channel, 16000 sample rate, got:",
			format.BitsPerSample, format.NumChannels, format.SampleRate)
		return
	}

	var data []int16
	for {
		samples, err := reader.ReadSamples()
		if err != nil {
			if err != io.EOF {
				fmt.Println("Read error:", err)
			}
			break
		}
		for _, sample := range samples {
			data = append(data, int16(reader.IntValue(sample, 0)))
		}
	}
	fmt.Println("Read", len(data), "samples")

	var sender cloudproc.Sender
	if *makeproc {
		cloudproc.SetVerbose(*verbose)
		options := []cloudproc.Option{cloudproc.WithCompression(*compress)}
		if *ms {
			options = append(options, cloudproc.WithHandler(cloudproc.HandlerMicrosoft))
		}
		proc, err := harness.CreateMemProcess(options...)
		if err != nil {
			fmt.Println("Couldn't create test cloud process:", err)
			return
		}
		defer proc.Close()
		sender = proc

		wg := sync.WaitGroup{}
		go func() {
			wg.Add(1)
			defer wg.Done()
			intent, _ := proc.ReadIntent()
			fmt.Println("Got AI response:", intent)
		}()
		defer wg.Wait()
	} else {
		conn, err := ipc.NewUnixgramClient(ipc.GetSocketPath("cp_test"), "wavtester")
		if err != nil {
			fmt.Println("Couldn't connect to cloud client:", err)
			return
		}
		defer conn.Close()
		sender = cloudproc.NewIpcSender(conn)
	}

	// simulate real-time recording and delay between each send
	const chunkMs = 60
	interval := time.Millisecond * chunkMs
	nextSend := time.Now().Add(interval)

	// send hotword
	fmt.Println("Sent: 0 samples")
	sender.SendMessage(cloudproc.HotwordMessage)
	buf := data
	sent := 0
	const chunkSamples = 16000 / 1000 * chunkMs
	for len(buf) >= chunkSamples {
		sleepTarget := nextSend.Sub(time.Now())
		nextSend = nextSend.Add(interval)
		time.Sleep(sleepTarget)

		temp := buf[:chunkSamples]
		buf = buf[chunkSamples:]

		data := &bytes.Buffer{}
		binary.Write(data, binary.LittleEndian, temp)

		n, err := sender.SendAudio(data.Bytes())
		if n != len(data.Bytes()) || err != nil {
			fmt.Println("Expected to send", len(data.Bytes()), "but instead sent", n, "with error:", err)
		}
		sent += len(temp)
		fmt.Println("\rSent:", sent, "samples")
	}
	fmt.Println("Done sending, exiting")

}
