package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"

	"github.com/google/uuid"
)

var client = &http.Client{}

// CloudResponse holds data returned from the server each time audio is sent. Result
// will be nil unless IsFinal is true, in which case Result should hold a valid CloudResult
// structure.
type CloudResponse struct {
	IsFinal bool   `json:"is_final"`
	Err     string `json:"error"`
	Result  *CloudResult
}

// CloudResult contains the data associated with a final response from the server.
type CloudResult struct {
	QueryText        string  `json:"query_text"`
	SpeechConfidence float64 `json:"speech_confidence"`
	Action           string  `json:"action"`
	IntentConfidence float64 `json:"intent_confidence"`
}

// CloudContext stores session information for a given sequence of streamed audio
// data. When first sending audio data to the server, a new context should be obtained
// via NewCloudContext and data sent via the StreamData method.
type CloudContext struct {
	session string
	device  string
}

// NewCloudContext returns a new context with a randomly-generated session UUID
// and the given device string.
func NewCloudContext(device string) *CloudContext {
	return &CloudContext{
		session: uuid.New().String(),
		device:  device}
}

const url = "https://127.0.0.1"

// StreamData sends the given buffer of audio byte data to the server. The same CloudContext
// should be re-used multiple times until the returned CloudResponse has its IsFinal field
// set to true, at which point a new session should be used for further audio requests.
func (c *CloudContext) StreamData(buf []byte) *CloudResponse {
	// Create request
	req, err := http.NewRequest("POST", url+"/1/stream-detect-intent", bytes.NewReader(buf))
	if err != nil {
		fmt.Println("Error creating HTTP request:", err)
		return nil
	}
	req.Header.Set("Content-Type", "application/octet-stream")
	req.Header.Set("Device-Id", c.device)
	req.Header.Set("Session-Id", c.session)
	req.Header.Set("Authorization", "MAGIC KEY")

	// Execute request
	resp, err := client.Do(req)
	if err != nil {
		fmt.Println("Error executing HTTP request:", err)
		return nil
	}

	// Parse response
	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		fmt.Println("Error reading buffer:", err)
		return nil
	}
	cloudResp := CloudResponse{}
	err = json.Unmarshal(body, &resp)
	if err != nil {
		fmt.Println("Error unmarshaling cloud response:", err)
		return nil
	}
	if cloudResp.IsFinal {
		*cloudResp.Result = CloudResult{}
		err = json.Unmarshal(body, &cloudResp.Result)
		if err != nil {
			fmt.Println("Error unmarshaling cloud result:", err)
			cloudResp.Result = nil
		}
	}

	return &cloudResp
}
