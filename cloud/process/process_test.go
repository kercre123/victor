package main

import (
	"anki/ipc"
	"fmt"
	"math/rand"
	"testing"
	"time"
)

const (
	aiPort  = 17468
	micPort = 17469
)

var serverCount = 0

func fakeCloudHandler(buf []byte) (*CloudResponse, error) {
	serverCount++
	if serverCount <= 1 {
		return &CloudResponse{
			IsFinal: false,
			Err:     "",
			Result:  nil}, nil
	}
	return &CloudResponse{
		IsFinal: true,
		Err:     "",
		Result: &CloudResult{
			QueryText:        "How are you feeling?",
			SpeechConfidence: 0.5,
			Action:           "murder",
			IntentConfidence: 1.0}}, nil
}

func TestProcess(t *testing.T) {
	// inject test hook
	cloudTest = fakeCloudHandler

	aiServer, err1 := ipc.NewServerSocket(aiPort)
	micServer, err2 := ipc.NewServerSocket(micPort)

	time.Sleep(100 * time.Millisecond)

	aiClient, err3 := ipc.NewClientSocket("0.0.0.0", aiPort)
	micClient, err4 := ipc.NewClientSocket("0.0.0.0", micPort)

	if err1 != nil || err2 != nil || err3 != nil || err4 != nil {
		t.Fatal("Socket error")
	}

	go runProcess(micClient, aiClient)

	aiChan := make(chan socketMsg)
	micChan := make(chan socketMsg)
	go socketReader(aiServer, aiChan)
	go socketReader(micServer, micChan)

	time.Sleep(100 * time.Millisecond)

	micServer.Write([]byte("hotword"))
	for _ = range [15]byte{} {
		time.Sleep(10 * time.Millisecond)
		buf := make([]byte, streamSize/10+1)
		rand.Read(buf) // fill with random bytes
		micServer.Write(buf)
	}

	// our channels should still be waiting
	select {
	case <-aiChan:
		t.Fatal("Received AI message on socket before server finished")
	case <-micChan:
		t.Fatal("Received Mic message on socket before server finished")
	default:
	}

	// server should have responded once already with a "not done yet"
	if serverCount != 1 {
		t.Fatal("Server count wrong, is", serverCount)
	}
	// keep sending
	for _ = range [5]byte{} {
		time.Sleep(10 * time.Millisecond)
		buf := make([]byte, streamSize/10+32)
		rand.Read(buf) // fill with random bytes
		micServer.Write(buf)
	}

	time.Sleep(100 * time.Millisecond)

	// now we should have gotten an answer
	for _ = range [2]byte{} {
		select {
		case msg := <-aiChan:
			if string(msg.buf) != "murder" {
				t.Fatal("Wrong message on AI channel:", string(msg.buf))
			} else {
				fmt.Println("Got correct AI message")
			}
		case msg := <-micChan:
			if msg.n != 1 || msg.buf[0] != 0 {
				t.Fatal("Wrong message back to mic channel:", msg.buf)
			} else {
				fmt.Println("Got correct mic message")
			}
		default:
			t.Fatal("Shouldn't have idle channels after send complete")
		}
	}
}
