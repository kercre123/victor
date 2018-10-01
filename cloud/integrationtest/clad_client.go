package main

import (
	"anki/ipc"
	"fmt"
)

type cladClient struct {
	conn ipc.Conn
}

func (c *cladClient) connect(socketName string) error {
	name := ipc.GetSocketPath(socketName)

	var err error
	c.conn, err = ipc.NewUnixgramClient(name, "cli")
	if err != nil {
		fmt.Println("Couldn't create socket", name, ":", err)
	}

	return err
}
