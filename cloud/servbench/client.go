package main

import (
	"anki/ipc/multi"
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"math/rand"
	"os"
	"sync"
	"time"
)

func client(proto string, name string, iter int64, rt bool) {
	wg := sync.WaitGroup{}
	if name != "send" && name != "altrecv" {
		name = "recv"
	}
	underClient, err := getClient(proto, name)
	if err != nil {
		fmt.Println("error creating client:", err)
		return
	}

	var recvfunc func() []byte
	var sendfunc func([]byte) (int, error)
	if name == "altrecv" {
		recvfunc = underClient.ReadBlock
		sendfunc = underClient.Write
	} else {
		client, err := multi.NewClient(underClient, name)
		if err != nil {
			fmt.Println(err)
			return
		}
		defer client.Close()

		recvfunc = func() (b []byte) {
			_, b, _ = client.ReceiveBlock()
			return
		}
		sendfunc = func(b []byte) (int, error) {
			return client.Send("recv", b)
		}
	}

	if name == "send" {
		if rt {
			wg.Add(1)
			go receiver(recvfunc, func() { wg.Done() }, nil)
		}
		sender(sendfunc, iter)
	} else {
		var rtsend func([]byte)
		if rt {
			rtsend = func(b []byte) {
				sendfunc(b)
			}
		}
		receiver(recvfunc, nil, rtsend)
	}
	wg.Wait()
}

func sender(sendfunc func([]byte) (int, error), count int64) {
	fmt.Printf("Sender is ready to iterate %v times, press enter to start\n", count)
	reader := bufio.NewReader(os.Stdin)
	reader.ReadLine()

	for i := int64(0); i < count; i++ {
		buf := make([]byte, 1016)
		rand.Read(buf)
		timebuf := bytes.NewBuffer(make([]byte, 0, 8))
		t := time.Now()
		binary.Write(timebuf, binary.LittleEndian, t.UnixNano())
		sendbuf := append(timebuf.Bytes(), buf...)

		n, err := sendfunc(sendbuf)
		if n != len(sendbuf) || err != nil {
			fmt.Println("Send error:", i, n, err)
			break
		}
	}
	sendfunc([]byte{0})
}

type msg struct {
	buf []byte
	t   time.Time
}

func receiver(recvfunc func() []byte, donefunc func(), sendfunc func([]byte)) {
	if donefunc != nil {
		defer donefunc()
	}
	fmt.Println("Receiver is ready")
	sendback := sendfunc != nil

	msgbuf := make([]msg, 0, 1000000)

	for i := 0; ; i++ {
		buf := recvfunc()
		if sendback {
			sendfunc(buf)
		}
		if len(buf) < 2 {
			fmt.Println("Done after receives:", i)
			break
		}
		if !sendback {
			msgbuf = msgbuf[:len(msgbuf)+1]
			msgbuf[i] = msg{buf, time.Now()}
		}
	}

	if !sendback {
		printdata(msgbuf)
	}
}

func printdata(msgbuf []msg) {
	var totaldiff int64
	var totalsize int64
	for i := 0; i < len(msgbuf); i++ {
		var nano int64
		buf := bytes.NewBuffer(msgbuf[i].buf)
		binary.Read(buf, binary.LittleEndian, &nano)

		sent := time.Unix(0, nano)
		diff := msgbuf[i].t.Sub(sent)

		totaldiff += int64(diff)
		totalsize += int64(len(msgbuf[i].buf))
	}

	fmt.Println("total:", totaldiff, "iters:", len(msgbuf), "size in bytes:", totalsize)
	fmt.Println("Average ns delay:", float64(totaldiff)/float64(len(msgbuf)))
	fmt.Println("Average ms delay:", (float64(totaldiff)/float64(len(msgbuf)))/float64(time.Millisecond))
}
