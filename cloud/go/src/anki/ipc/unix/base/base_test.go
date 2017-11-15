package base

import "testing"
import "time"
import "fmt"

func TestBase(t *testing.T) {
	path := "/tmp/asdf"
	server, err := NewUnixServer(path)
	if err != nil {
		t.Fatal("Server creation error:", err)
	}
	defer server.Close()
	c1, err1 := NewUnixClient(path)
	c2, err2 := NewUnixClient(path)
	if err1 != nil || err2 != nil {
		t.Fatal("Client creation error:", err1, err2)
	}
	defer c1.Close()
	defer c2.Close()

	go func() {
		i := 0
		for conn := range server.NewConns() {
			go func(conn Conn, i int) {
				for {
					b := conn.ReadBlock()
					if b == nil {
						return
					}
					fmt.Println("Got msg on server conn", i, ":", string(b))
				}
			}(conn, i)
			i++
		}
	}()

	c1.Write([]byte("asdf1"))
	c2.Write([]byte("asdf2"))
	c1.Write([]byte("stuff1"))
	c1.Write([]byte("three1"))
	c2.Write([]byte("aa2"))
	c2.Write([]byte("bb2"))
	time.Sleep(500 * time.Millisecond)
}
