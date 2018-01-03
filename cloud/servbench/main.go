package main

import (
	"anki/ipc"
	"anki/ipc/multi"
	"bufio"
	"flag"
	"fmt"
	"os"
	"sync"
)

const portNum = 12798

func getClient(proto string, name string) (ipc.Conn, error) {
	switch proto {
	case "udp":
		return ipc.NewUDPClient("0.0.0.0", portNum)
	case "unixstream":
		return ipc.NewUnixClient("pipeasdf")
	case "unixgram":
		return ipc.NewUnixgramClient("pipeasdf", name)
	case "unixpacket":
		return ipc.NewUnixPacketClient("pipeasdf")
	default:
		panic("asdf")
	}
}

func getServer(proto string) (ipc.Server, error) {
	switch proto {
	case "udp":
		return ipc.NewUDPServer(portNum)
	case "unixstream":
		return ipc.NewUnixServer("pipeasdf")
	case "unixgram":
		return ipc.NewUnixgramServer("pipeasdf")
	case "unixpacket":
		return ipc.NewUnixPacketServer("pipeasdf")
	default:
		panic("asdf")
	}
}

func main() {
	var proto string
	var mode string
	var iter int64
	var rt bool
	flag.StringVar(&proto, "type", "", "type to use - udp, unixstream, unixgram, unixpacket (default unixstream)")
	flag.StringVar(&mode, "mode", "server", "operation mode - server, send, recv, altrecv, altserv")
	flag.Int64Var(&iter, "iter", 10, "number of times to iterate")
	flag.BoolVar(&rt, "rt", false, "round trip?")
	flag.Parse()

	if mode != "server" && mode != "altserv" {
		client(proto, mode, iter, rt)
		return
	}

	server, err := getServer(proto)
	if err != nil {
		fmt.Println(err)
		return
	}

	if mode == "altserv" {
		directsend(server, iter, rt)
		return
	}

	serv, err := multi.NewServer(server)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer serv.Close()
	fmt.Println("Server started")
	reader := bufio.NewReader(os.Stdin)
	reader.ReadString('\n')
}

func directsend(serv ipc.Server, iter int64, rt bool) {
	conn := <-serv.NewConns()
	wg := sync.WaitGroup{}
	if rt {
		wg.Add(1)
		go receiver(func() []byte {
			return conn.ReadBlock()
		}, func() { wg.Done() }, nil)
	}
	sender(func(b []byte) (int, error) {
		return conn.Write(b)
	}, iter)
	wg.Wait()
}
