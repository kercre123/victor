package ipc

import (
	"testing"
	"time"
)

func blockTest(t *testing.T, sA, sB Socket) {
	// start blocking read; shouldn't finish yet
	ch := make(chan int)
	go func() {
		_, b := sA.ReadBlock()
		ch <- 1
		if string(b) != "Hello!" {
			t.Fatal("Socket A failed to receive message correctly:", string(b))
		}
	}()
	time.Sleep(500 * time.Millisecond)
	select {
	case <-ch:
		t.Fatal("ReadBlock() didn't block")
	default:
	}

	// now send from B
	n, err := sB.Write([]byte("Hello!"))
	if n != 6 || err != nil {
		t.Fatal("Socket B send:", n, err)
	}
	time.Sleep(100 * time.Millisecond)

	// A should have received it now
	select {
	case <-ch:
	default:
		t.Fatal("Socket A didn't read message")
	}
}

func TestSocket(t *testing.T) {
	server, err := NewServerSocket(21555)
	if err != nil {
		t.Fatal("Server error:", err)
	}

	time.Sleep(200 * time.Millisecond)
	client, err := NewClientSocket("127.0.0.1", 21555)
	if err != nil {
		t.Fatal("Client error:", err)
	}

	if n, _ := server.Read(); n != 0 {
		t.Fatal("Initial server read didn't return 0")
	}
	if n, _ := client.Read(); n != 0 {
		t.Fatal("Initial client read didn't return 0")
	}

	blockTest(t, client, server)
	blockTest(t, server, client)

	server.Close()
	client.Close()
}
