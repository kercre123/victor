package main

import (
	"anki/voice"
	"clad/cloud"
	"fmt"
)

type micClient struct {
	msgIO voice.MsgIO
}

func (c *micClient) connectionCheck() error {
	requestMsg := cloud.NewMessageWithConnectionCheck(nil)

	err := c.msgIO.Send(requestMsg)
	if err != nil {
		return err
	}

	responseMsg, err := c.msgIO.Read()
	if err != nil {
		return err
	}

	switch responseMsg.Tag() {
	case cloud.MessageTag_ConnectionResult:
		_ = responseMsg.GetConnectionCheck()
		return nil
	case cloud.MessageTag_Error:
		errResponse := responseMsg.GetError()
		return fmt.Errorf("MicError: %v", errResponse.Error)
	}
	return fmt.Errorf("Major error: received unknown tag %d", responseMsg.Tag())
}
