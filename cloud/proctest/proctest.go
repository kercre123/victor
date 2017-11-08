package main

// typedef void (*voidFunc) ();
import "C"
import (
	"anki/cloudproc"
	"anki/ipc"
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"strings"
	"time"
)

const (
	aiPort  = 12345
	micPort = 9880
)

type appData struct {
	sock        ipc.Socket
	sampleCount int
}

var app *appData

func (a *appData) audioCallback(samples []int16) {
	a.sampleCount += len(samples)
	fmt.Print("\rSamples recorded: ", a.sampleCount)

	data := &bytes.Buffer{}
	binary.Write(data, binary.LittleEndian, samples)

	_, err := a.sock.Write(data.Bytes())
	if err != nil {
		fmt.Println("\nWrite error:", err)
	}
}

//export GoAudioCallback
func GoAudioCallback(cSamples []int16) {
	// this is temporary memory in C; need to bring it into Go's world
	samples := make([]int16, len(cSamples))
	copy(samples, cSamples)
	app.audioCallback(samples)
}

//export GoMain
func GoMain(startRecording, stopRecording C.voidFunc) {
	_, err1 := ipc.NewServerSocket(aiPort)
	micServer, err2 := ipc.NewServerSocket(micPort)

	time.Sleep(100 * time.Millisecond)

	aiClient, err3 := ipc.NewClientSocket("0.0.0.0", aiPort)
	micClient, err4 := ipc.NewClientSocket("0.0.0.0", micPort)
	defer aiClient.Close()
	defer micClient.Close()

	if err1 != nil || err2 != nil || err3 != nil || err4 != nil {
		fmt.Println("Socket error")
		return
	}

	kill := make(chan struct{})
	go cloudproc.RunProcess(micClient, aiClient, kill)
	defer close(kill)

	for {
		app = &appData{micServer, 0}
		fmt.Println("Press enter to start recording! (type \"done\" to quit)")
		r := bufio.NewReader(os.Stdin)
		str, _ := r.ReadString('\n')

		if strings.TrimSpace(str) == "done" {
			break
		}

		_, err := micServer.Write([]byte("hotword"))
		if err != nil {
			fmt.Println("Hotword error:", err)
			break
		}
		runCFunc(startRecording)

		n, _ := micServer.ReadBlock()
		runCFunc(stopRecording)
		if n != 2 {
			fmt.Println("Unexpected mic response size", n)
			break
		}
	}

}

func main() {}
