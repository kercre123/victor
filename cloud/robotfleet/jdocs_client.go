package main

import (
	"bytes"
	"clad/cloud"
	"fmt"
)

type jdocsClient struct {
	cladClient
}

func (c *jdocsClient) requestResponse(request *cloud.DocRequest) (*cloud.DocResponse, error) {
	var requestBuf bytes.Buffer
	if err := request.Pack(&requestBuf); err != nil {
		return nil, err
	}

	if _, err := c.conn.Write(requestBuf.Bytes()); err != nil {
		return nil, err
	}

	responseBuf := c.conn.ReadBlock()
	var responseMsg cloud.DocResponse
	if err := responseMsg.Unpack(bytes.NewBuffer(responseBuf)); err != nil {
		return nil, err
	}

	return &responseMsg, nil
}

func (c *jdocsClient) Write(request *cloud.WriteRequest) (*cloud.WriteResponse, error) {
	responseMessage, err := c.requestResponse(cloud.NewDocRequestWithWrite(request))

	if err != nil {
		return nil, err
	}

	switch responseMessage.Tag() {
	case cloud.DocResponseTag_Write:
		response := responseMessage.GetWrite()
		return response, nil
	case cloud.DocResponseTag_Err:
		err := responseMessage.GetErr().Err
		return nil, fmt.Errorf("Major error: %v", err)
	}
	return nil, fmt.Errorf("Major error: received unknown tag %d", responseMessage.Tag())
}

func (c *jdocsClient) Read(request *cloud.ReadRequest) (*cloud.ReadResponse, error) {
	responseMessage, err := c.requestResponse(cloud.NewDocRequestWithRead(request))

	if err != nil {
		return nil, err
	}

	switch responseMessage.Tag() {
	case cloud.DocResponseTag_Read:
		response := responseMessage.GetRead()
		return response, nil
	case cloud.DocResponseTag_Err:
		err := responseMessage.GetErr().Err
		return nil, fmt.Errorf("Major error: %v", err)
	}
	return nil, fmt.Errorf("Major error: received unknown tag %d", responseMessage.Tag())
}

func (c *jdocsClient) Delete() error {
	// TODO: not yet implemented
	return nil
}
