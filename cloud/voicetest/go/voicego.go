package main

// typedef void (*voidFunc) ();
import "C"
import (
	"bufio"
	"bytes"
	"context"
	"encoding/binary"
	"fmt"
	"os"
	"strings"

	"anki/cloudproc"

	"github.com/anki/sai-chipper-voice/client/chipper"
	api "github.com/anki/sai-go-util/http/apiclient"
	"github.com/google/uuid"
	wav "github.com/youpy/go-wav"
)

type appData struct {
	samples     []int16
	sampleCount int
	lastSend    int
	tail        []int16
	client      *chipper.ChipperClient
	notice      chan struct{}
	dataStream  chan []byte
	first       bool
}

var app = appData{}

func (a *appData) audioRoutine() {
	for data := range a.dataStream {
		err := a.client.SendAudio(data)
		if err != nil {
			fmt.Println("Error sending audio:", err)
			close(a.dataStream)
			return
		}
		if a.first {
			a.first = false
			close(a.notice)
		}
	}
}

func (a *appData) AudioCallback(samples []int16) {
	a.sampleCount += len(samples)
	a.samples = append(a.samples, samples...)
	a.tail = a.samples[a.lastSend:]
	fmt.Print("\rSamples received: ", a.sampleCount)

	if len(app.tail) < 1600 {
		return
	}

	samples = a.tail[:1600]
	a.lastSend += 1600

	data := &bytes.Buffer{}
	binary.Write(data, binary.LittleEndian, samples)
	if data.Len() != 3200 {
		fmt.Println("WTF")
	}
	a.dataStream <- data.Bytes()
}

//export GoAudioCallback
func GoAudioCallback(cSamples []int16) {
	// this is temporary memory in C; need to bring it into Go's world
	samples := make([]int16, len(cSamples))
	copy(samples, cSamples)
	app.AudioCallback(samples)
}

//export GoMain
func GoMain(startRecording, stopRecording C.voidFunc) {
	client, err := chipper.NewClient("", cloudproc.ChipperSecret, "device-id",
		uuid.New().String()[:16],
		api.WithServerURL(cloudproc.ChipperURL))
	if err != nil {
		fmt.Println("Error starting chipper:", err)
		return
	}
	app.notice = make(chan struct{})
	app.dataStream = make(chan []byte, 40)
	app.client = client
	app.first = true

	fmt.Println("Press enter to start recording!")
	r := bufio.NewReader(os.Stdin)
	_, _ = r.ReadString('\n')

	go app.audioRoutine()
	runCFunc(startRecording)
	<-app.notice

	ctx := context.Background()
	intent, err := client.WaitForIntent(ctx)
	if err != nil {
		fmt.Println("Intent read failed", err)
		return
	}

	runCFunc(stopRecording)
	close(app.dataStream)

	fmt.Println("")
	fmt.Println("Stopped recording")
	fmt.Println("Intent:", intent)

	fmt.Println("Insert filename to save to: ")
	filename, _ := r.ReadString('\n')
	filename = strings.TrimSpace(filename)
	if filename == "" {
		return
	}

	f, _ := os.Create(filename)
	defer func() {
		err := f.Close()
		if err != nil {
			fmt.Println("File close error:", err)
		}
	}()

	bufWriter := bufio.NewWriter(f)
	writer := wav.NewWriter(bufWriter, uint32(len(app.samples)), 1, 16000, 16)
	err = writer.WriteSamples(convertSamples(app.samples))
	if err != nil {
		fmt.Println("Sample write error:", err)
	}
	err = bufWriter.Flush()
	if err != nil {
		fmt.Println("Buffer flush error:", err)
	}
	err = f.Sync()
	if err != nil {
		fmt.Println("File sycn error:", err)
	}
}

func convertSamples(inSamples []int16) (samples []wav.Sample) {
	// the wav library, despite the fact that not all audio is stereo,
	// uses a struct of two values to represent a sample
	samples = make([]wav.Sample, len(inSamples))
	for i, val := range inSamples {
		samples[i].Values[0] = int(val)
	}
	return
}

// apparently when doing -buildmode=c-archive, we need main()
// I have no clue why
func main() {}
