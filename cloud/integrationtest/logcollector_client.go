package main

import (
	"bytes"
	"clad/cloud"
	"fmt"
)

type logcollectorClient struct {
	cladClient
}

func (c *logcollectorClient) requestResponse(request *cloud.LogCollectorRequest) (*cloud.LogCollectorResponse, error) {
	var requestBuf bytes.Buffer
	if err := request.Pack(&requestBuf); err != nil {
		return nil, err
	}

	if _, err := c.conn.Write(requestBuf.Bytes()); err != nil {
		return nil, err
	}

	responseBuf := c.conn.ReadBlock()
	var responseMsg cloud.LogCollectorResponse
	if err := responseMsg.Unpack(bytes.NewBuffer(responseBuf)); err != nil {
		return nil, err
	}

	return &responseMsg, nil
}

func (c *logcollectorClient) upload(logFileName string) (string, error) {
	requestMsg := cloud.NewLogCollectorRequestWithUpload(&cloud.UploadRequest{LogFileName: logFileName})

	responseMsg, err := c.requestResponse(requestMsg)
	if err != nil {
		return "", err
	}

	switch responseMsg.Tag() {
	case cloud.LogCollectorResponseTag_Upload:
		uploadResponse := responseMsg.GetUpload()
		return uploadResponse.LogUrl, nil
	case cloud.LogCollectorResponseTag_Err:
		errResponse := responseMsg.GetErr()
		return "", fmt.Errorf("LogCollectorError: %v", errResponse.Err)
	}
	return "", fmt.Errorf("Major error: received unknown tag %d", responseMsg.Tag())
}

func newLogcollectorClient() *logcollectorClient {
	return new(logcollectorClient)
}
