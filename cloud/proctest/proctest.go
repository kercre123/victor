package main

// typedef void (*voidFunc) ();
import "C"
import (
	"anki/cloudproc"
	"anki/ipc"
	"bufio"
	"bytes"
	"encoding/binary"
	"flag"
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
	sock        ipc.Conn
	sampleCount int
	verbose     bool
}

var app *appData

func (a *appData) audioCallback(samples []int16) {
	a.sampleCount += len(samples)
	if !a.verbose {
		// if verbose is on this will format terribly
		fmt.Print("\rSamples recorded: ", a.sampleCount)
	}

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
	aiServer, err1 := ipc.NewUDPServer(aiPort)
	micServerDaemon, err2 := ipc.NewUDPServer(micPort)
	defer aiServer.Close()
	defer micServerDaemon.Close()

	var verbose bool
	flag.BoolVar(&verbose, "verbose", false, "enable verbose logging")
	flag.Parse()

	time.Sleep(100 * time.Millisecond)

	aiClient, err3 := ipc.NewUDPClient("0.0.0.0", aiPort)
	micClient, err4 := ipc.NewUDPClient("0.0.0.0", micPort)
	defer aiClient.Close()
	defer micClient.Close()

	if err1 != nil || err2 != nil || err3 != nil || err4 != nil {
		fmt.Println("Socket error")
		return
	}

	kill := make(chan struct{})
	cloudproc.SetVerbose(verbose)

	receiver := cloudproc.NewIpcReceiver(micClient, kill)
	process := &cloudproc.Process{}
	process.AddReceiver(receiver)
	process.AddIntentWriter(aiClient)
	go process.Run(kill)
	defer close(kill)

	micServer := <-micServerDaemon.NewConns()

	for {
		app = &appData{micServer, 0, verbose}
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

		buf := micServer.ReadBlock()
		runCFunc(stopRecording)
		if len(buf) != 2 {
			fmt.Println("Unexpected mic response size", len(buf))
			break
		}
	}

}

func main() {}
