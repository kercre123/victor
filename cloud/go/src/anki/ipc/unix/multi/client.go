package multi

import (
	"anki/ipc/unix/base"
	"errors"
	"strings"
)

type UnixClient struct {
	client base.Conn
	name   string
}

func receiveInternal(receiveFunc func() []byte) (from string, buf []byte, err error) {
	buf = receiveFunc()
	if buf == nil || len(buf) == 0 {
		return "", nil, nil
	}
	nullIdx := strings.Index(string(buf), "\x00")
	if nullIdx <= 0 {
		return "", nil, errors.New("no null identifier in buf")
	}
	from = string(buf[:nullIdx])
	buf = buf[nullIdx+1:]
	return
}

func (c *UnixClient) Receive() (from string, buf []byte, err error) {
	return receiveInternal(func() []byte {
		return c.client.Read()
	})
}

func (c *UnixClient) ReceiveBlock() (from string, buf []byte, err error) {
	return receiveInternal(func() []byte {
		return c.client.ReadBlock()
	})
}

func (c *UnixClient) Send(dest string, buf []byte) (int, error) {
	sendbuf := getBufferForMessage(dest, buf)
	return c.client.Write(sendbuf)
}

func (c *UnixClient) Close() error {
	return c.client.Close()
}

func NewUnixClient(path string, clientName string) (*UnixClient, error) {
	baseClient, err := base.NewUnixClient(path)
	if err != nil {
		return nil, err
	}
	client := &UnixClient{baseClient, clientName}

	client.client.Write([]byte(clientName))

	return client, nil
}
