package ipc

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
)

type streamConn struct {
	io.ReadWriteCloser
}

func (c *streamConn) Read(b []byte) (int, error) {
	var buflen uint32
	err := binary.Read(c.ReadWriteCloser, binary.BigEndian, &buflen)
	if err != nil {
		return 0, err
	}
	if buflen > uint32(len(b)) {
		fmt.Println("buffer overflow, have size", len(b), "but need", buflen)
		buflen = uint32(len(b))
	}
	return c.ReadWriteCloser.Read(b[:buflen])
}

func (c *streamConn) Write(b []byte) (int, error) {
	lenbuf := bytes.NewBuffer(make([]byte, 0, 4))
	binary.Write(lenbuf, binary.BigEndian, uint32(len(b)))
	_, err := c.ReadWriteCloser.Write(lenbuf.Bytes())
	if err != nil {
		return 0, err
	}
	return c.ReadWriteCloser.Write(b)
}

func newStreamConnection(conn io.ReadWriteCloser) io.ReadWriteCloser {
	return &streamConn{conn}
}
