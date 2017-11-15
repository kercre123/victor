package multi

import "testing"
import "time"
import "fmt"
import "runtime"

func TestMulti(t *testing.T) {
	defer func() {
		// make sure no goroutines are hanging
		fmt.Println("not a panic, shutting down:")
		buf := make([]byte, 32768)
		n := runtime.Stack(buf, true)
		fmt.Print(string(buf[:n]))
	}()

	path := "/tmp/asdf"
	server, err := NewUnixServer(path)
	if err != nil {
		t.Fatal("Server creation error:", err)
	}
	defer server.Close()
	c1, err1 := NewUnixClient(path, "c1")
	c2, err2 := NewUnixClient(path, "c2")
	if err1 != nil || err2 != nil {
		t.Fatal("Client creation error:", err1, err2)
	}
	defer c1.Close()
	defer c2.Close()

	// start routines for clients to print messages as they receive them
	for _, conn := range []*UnixClient{c1, c2} {
		go func(client *UnixClient) {
			for {
				from, buf, _ := client.ReceiveBlock()
				if buf == nil {
					return
				}
				fmt.Println(client.name, "from", from, ":", string(buf))
			}
		}(conn)
	}

	c1.Send("c2", []byte("asdf1"))
	c2.Send("c1", []byte("asdf2"))
	c1.Send("c2", []byte("stuff1"))
	c1.Send("c2", []byte("three1"))
	c2.Send("c1", []byte("aa2"))
	c2.Send("c1", []byte("bb2"))
	time.Sleep(500 * time.Millisecond)
}
