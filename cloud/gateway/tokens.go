package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"time"

	"anki/ipc"
	"anki/log"
	"anki/robot"
	cloud_clad "clad/cloud"

	"github.com/anki/sai-token-service/client/token"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
)

const (
	jdocDomainSocket = "jdocs_server"
	jdocSocketSuffix = "gateway_client"
)

// ClientToken holds the tuple of the client token hash and the
// user-visible client name (e.g. Adam's iPhone)
type ClientToken struct {
	Hash       string `json:"hash"`
	ClientName string `json:"client_name"`
	AppId      string `json:"app_id"`
	IssuedAt   string `json:"issued_at"`
}

// ClientTokenManager holds all the client token tuples for a given
// userid+robot, along with a handle to the Jdocs service document
// that stores them.
// Note: comes from the ClientTokenDocument definition
type ClientTokenManager struct {
	ClientTokens     []ClientToken `json:"client_tokens"`
	jdocIPC          IpcManager    `json:"-"`
	updateChan       chan struct{} `json:"-"`
	recentTokenIndex int           `json:"-"`
}

func (ctm *ClientTokenManager) Init() error {
	ctm.updateChan = make(chan struct{})
	ctm.jdocIPC.Connect(ipc.GetSocketPath(jdocDomainSocket), jdocSocketSuffix)
	ctm.UpdateTokens()
	return nil
}

func (ctm *ClientTokenManager) Close() error {
	return ctm.jdocIPC.Close()
}

func (ctm *ClientTokenManager) CheckToken(clientToken string) (string, error) {
	if len(ctm.ClientTokens) == 0 {
		return "", grpc.Errorf(codes.Unauthenticated, "no valid tokens")
	}
	recentToken := ctm.ClientTokens[ctm.recentTokenIndex]
	err := token.CompareHashAndToken(recentToken.Hash, clientToken)
	if err == nil {
		return recentToken.ClientName, nil
	}
	for idx, validToken := range ctm.ClientTokens {
		if idx == ctm.recentTokenIndex || len(validToken.Hash) == 0 {
			continue
		}
		err = token.CompareHashAndToken(validToken.Hash, clientToken)
		if err == nil {
			ctm.recentTokenIndex = idx
			return validToken.ClientName, nil
		}
	}
	return "", grpc.Errorf(codes.Unauthenticated, "invalid token")
}

// DecodeTokenJdoc will update existing valid tokens, from a jdoc received from the server
func (ctm *ClientTokenManager) DecodeTokenJdoc(jdoc []byte) error {
	ctm.recentTokenIndex = 0
	return json.Unmarshal(jdoc, ctm)
}

// UpdateTokens polls the server for new tokens, and will update as necessary
func (ctm *ClientTokenManager) UpdateTokens() error {
	id, esn, err := ctm.getIDs()
	if err != nil {
		return err
	}
	resp, err := ctm.sendBlock(cloud_clad.NewDocRequestWithRead(&cloud_clad.ReadRequest{
		Account: id,
		Thing:   fmt.Sprintf("vic:%s", esn),
		Items: []cloud_clad.ReadItem{
			cloud_clad.ReadItem{
				DocName:      "vic.AppTokens",
				MyDocVersion: 0,
			},
		},
	}))
	if err != nil {
		return err
	}
	read := resp.GetRead()
	if read == nil {
		return fmt.Errorf("error while trying to read jdocs: %+v", resp)
	}
	if len(read.Items) == 0 {
		return errors.New("no jdoc in read response")
	}
	err = ctm.DecodeTokenJdoc([]byte(read.Items[0].Doc.JsonDoc))
	if err != nil {
		return nil
	}
	return nil
}

func (ctm *ClientTokenManager) getIDs() (string, string, error) {
	resp, err := ctm.sendBlock(cloud_clad.NewDocRequestWithUser(&cloud_clad.Void{}))
	if err != nil {
		return "", "", err
	}
	user := resp.GetUser()
	if user == nil {
		return "", "", fmt.Errorf("Unable to get robot's user id: %+v", resp)
	}
	esn, err := robot.ReadESN()
	if err != nil {
		return "", "", err
	}

	return user.UserId, esn, nil
}

func (ctm *ClientTokenManager) sendBlock(request *cloud_clad.DocRequest) (*cloud_clad.DocResponse, error) {
	// Write the request
	var err error
	var buf bytes.Buffer
	if request == nil {
		return nil, grpc.Errorf(codes.InvalidArgument, "Unable to parse request")
	}

	// The domain socket to vic-cloud does not have size in front of messages

	if err = request.Pack(&buf); err != nil {
		return nil, grpc.Errorf(codes.Internal, err.Error())
	}
	// TODO: use channel to a jdoc read/write manager goroutine
	_, err = ctm.jdocIPC.conn.Write(buf.Bytes())
	if err != nil {
		return nil, grpc.Errorf(codes.Internal, err.Error())
	}
	// Read the response
	msgBuffer := ctm.jdocIPC.conn.ReadBlock()
	if msgBuffer == nil {
		log.Println()
		return nil, grpc.Errorf(codes.Internal, "engine socket returned empty message")
	}
	var recvBuf bytes.Buffer
	recvBuf.Write(msgBuffer)
	msg := &cloud_clad.DocResponse{}
	if err := msg.Unpack(&recvBuf); err != nil {
		log.Printf("JdocIpcManager Call Err: %+v\n", err)
		return nil, grpc.Errorf(codes.Internal, err.Error())
	}
	return msg, nil
}

func (ctm *ClientTokenManager) updateTicker() {
	ticker := time.NewTicker(time.Minute * 5)
	for _ = range ticker.C {
		ctm.updateChan <- struct{}{}
	}
}

func (ctm *ClientTokenManager) StartUpdateListener() {
	go ctm.updateTicker()
	for {
		select {
		case <-ctm.updateChan:
			log.Println("Updating tokens...")
			ctm.UpdateTokens()
		}
	}
}
