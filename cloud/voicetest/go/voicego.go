package main

// typedef void (*voidFunc) ();
import "C"
import (
	"bufio"
	"fmt"
	"os"
	"strings"

	wav "github.com/youpy/go-wav"
)

type appData struct {
	samples []int16
}

var app = appData{}

func (a *appData) AudioCallback(samples []int16) {
	a.samples = append(a.samples, samples...)
	fmt.Print("\rSamples received: ", len(a.samples))
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
	fmt.Println("Press enter to start recording! (enter again to stop)")
	r := bufio.NewReader(os.Stdin)
	_, _ = r.ReadString('\n')
	runCFunc(startRecording)
	_, _ = r.ReadString('\n')
	runCFunc(stopRecording)
	fmt.Println("Stopped recording")
	fmt.Println("Insert filename to save to: ")
	filename, _ := r.ReadString('\n')
	filename = strings.TrimSpace(filename)
	if filename == "" {
		return
	}

	f, _ := os.Create(filename)
	defer f.Close()

	bufWriter := bufio.NewWriter(f)
	writer := wav.NewWriter(bufWriter, uint32(len(app.samples)), 1, 16000, 16)
	writer.WriteSamples(convertSamples(app.samples))
	bufWriter.Flush()
	f.Sync()
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
