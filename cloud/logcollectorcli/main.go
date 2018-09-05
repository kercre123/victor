package main

import (
	"bytes"
	"clad/cloud"
	"flag"
	"fmt"

	"anki/ipc"
)

func upload(conn ipc.Conn, logFileName string) (string, error) {
	requestMsg := cloud.NewLogCollectorRequestWithUpload(&cloud.UploadRequest{LogFileName: logFileName})

	var requestBuf bytes.Buffer
	if err := requestMsg.Pack(&requestBuf); err != nil {
		return "", err
	}

	if _, err := conn.Write(requestBuf.Bytes()); err != nil {
		return "", err
	}

	responseBuf := conn.ReadBlock()
	var responseMsg cloud.LogCollectorResponse
	if err := responseMsg.Unpack(bytes.NewBuffer(responseBuf)); err != nil {
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

func main() {
	var fileName string

	flag.StringVar(&fileName, "f", "/data/boot.log", "File name")
	flag.Parse()

	connection, err := ipc.NewUnixgramClient(ipc.GetSocketPath("logcollector_server"), "cli")
	if err != nil {
		fmt.Printf("Could not initialize server connection, error %v\n", err)
		return
	}
	defer connection.Close()

	fmt.Printf("Uploading file: %q\n", fileName)

	logURL, err := upload(connection, fileName)
	if err != nil {
		fmt.Printf("Could not upload: %v\n", err)
		return
	}

	fmt.Printf("Log file stored at: %q\n", logURL)
}
