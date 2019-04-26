package microsoft

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"io"
	"io/ioutil"
	"net/http"
	"net/url"
	"sort"
	"strings"
	"time"

	"github.com/anki/sai-go-util/log"
	"github.com/gorilla/websocket"

	cutil "github.com/anki/sai-chipper-voice/conversation/util"
)

const (
	tokenEndpoint             = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken"
	tokenRequestHeader        = "Ocp-Apim-Subscription-Key"
	webSocketHost             = "speech.platform.bing.com"
	websocketRequestPath      = "/speech/recognition/interactive/cognitiveservices/v1"
	turnEndString             = "Path:turn.end"
	hypothesisString          = "Path:speech.hypothesis"
	sendHeaderPath            = "Path:audio\r\n"
	sendHeaderContent         = "Content-Type:audio/x-wav\r\n"
	configHeaderPath          = "Path:speech.config\r\n"
	configHeaderContentType   = "Content-Type:application/json; charset=utf-8\r\n"
	luiBaseUrl                = "https://westus.api.cognitive.microsoft.com/luis/v2.0/apps/"
	microsoftMinimumThreshold = 0.1
	luisFallbackIntent        = "intent_system_unsupported"
)

type MicrosoftClient struct {
	bingKey      string
	languageCode string
	bearerToken  []byte
	conn         *websocket.Conn
}

type MicrosoftConfiguration struct {
	BingKey      string
	LanguageCode string
	RequestId    string
}

func NewMicrosoftClient(config *MicrosoftConfiguration) (*MicrosoftClient, error) {
	ms := &MicrosoftClient{
		bingKey:      config.BingKey,
		languageCode: config.LanguageCode,
	}

	token, err := ms.getAuthToken()
	if err != nil {
		alog.Warn{"action": "get_auth_token", "status": "unexpected_error", "error": err}.Log()
		return nil, err
	}
	ms.bearerToken = token

	// create websocket connection
	conn, err := ms.CreateWebSocket()
	if err != nil {
		alog.Warn{"action": "create_websocket_conn", "error": err}.Log()
		return nil, err
	}
	ms.conn = conn

	// send speech.config message
	ms.Initialize(config.RequestId)

	return ms, nil
}

func (ms *MicrosoftClient) getAuthToken() (token []byte, err error) {
	req, err := http.NewRequest("POST", tokenEndpoint, nil)
	if err != nil {
		alog.Warn{"action": "create_token_request", "status": "unexpected_error", "error": err}.Log()
		return
	}
	req.Header.Add(tokenRequestHeader, ms.bingKey)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		alog.Warn{"action": "get_jwt_auth", "status": "unexpected_error", "error": err}.Log()
		return
	}
	defer resp.Body.Close()

	token, err = ioutil.ReadAll(resp.Body)
	if err != nil {
		alog.Warn{"action": "copy_body_to_token", "status": "unexpected_error", "error": err}.Log()
		return
	}
	return
}

type MSHypothesis struct {
	Text     string
	Offset   int64
	Duration int64
}

func (ms *MicrosoftClient) serviceName() string {
	toReturn := "Microsoft-" + ms.languageCode
	return toReturn
}

func (ms *MicrosoftClient) CreateWebSocket() (*websocket.Conn, error) {
	header := http.Header{}
	header.Add("Authorization: Bearer", string(ms.bearerToken))
	header.Add("X-ConnectionId", "test")

	vals := url.Values{}
	vals.Add("language", ms.languageCode)
	u := url.URL{
		Scheme:   "wss",
		Host:     webSocketHost,
		Path:     websocketRequestPath,
		RawQuery: vals.Encode(),
	}

	c, resp, err := websocket.DefaultDialer.Dial(u.String(), header)
	if err != nil {
		body := []byte{}
		io.ReadFull(resp.Body, body)
		alog.Warn{"action": "connect_websocket", "status": resp.StatusCode, "response": string(body), "error": err}.Log()
		return nil, err
	}
	return c, nil
}

func (ms *MicrosoftClient) Initialize(session string) error {
	err := ms.sendSpeechConfig(session)
	if err != nil {
		alog.Warn{"action": "send_speech_config", "status": "unexpected_error", "error": err}.Log()
		return err
	}
	return nil
}

func (ms *MicrosoftClient) Close() error {
	return ms.Close()
}

func (ms *MicrosoftClient) GetSendHeader(session string) []byte {
	requestId := "X-RequestId:" + session
	timestamp := "X-Timestamp:" + time.Now().Format(time.RFC3339)
	return []byte(sendHeaderPath + timestamp + "\r\n" + sendHeaderContent + requestId + "\r\n\r\n")
}

func (ms *MicrosoftClient) SendAudio(audio []byte, header []byte) error {

	toStream := make([]byte, 2)
	binary.BigEndian.PutUint16(toStream, uint16(len(header)))
	toStream = append(toStream, header...)
	toStream = append(toStream, audio...)
	err := ms.conn.WriteMessage(websocket.BinaryMessage, toStream)
	if err != nil {
		alog.Warn{"action": "send_audio_stream", "status": "unexpected_error", "error": err}.Log()
		return err
	}
	return nil
}

func (ms *MicrosoftClient) RecvHypothesis() (string, bool, error) {
	_, message, err := ms.conn.ReadMessage()
	if err != nil {
		alog.Warn{"action": "recv_hypothesis", "status": "unexpected_error", "error": err}.Log()
		return "", false, err
	}

	messageStr := string(message)
	if strings.Contains(messageStr, hypothesisString) {
		incoming := MSHypothesis{}
		start := strings.Index(messageStr, "{")

		err = json.Unmarshal(message[start:], &incoming)
		if err != nil {
			alog.Warn{"action": "unmarshal_hypothesis", "status": "unexpected_error", "error": err}.Log()
			return "", false, err
		}

		hypothesis := incoming.Text
		return hypothesis, false, nil
	}

	if strings.Contains(messageStr, turnEndString) {
		return "", true, nil
	}
	return "", false, nil
}

type MSSpeechConfigDevice struct {
	Manufacturer string `json:"manufacturer"`
	Model        string `json:"model"`
	Version      string `json:"version"`
}

type MSSpeechConfigOs struct {
	Platform string `json:"platform"`
	Name     string `json:"name"`
	Version  string `json:"version"`
}

type MSSpeechConfigSystem struct {
	Version string `json:"version"`
}

type MSSpeechConfigContext struct {
	System MSSpeechConfigSystem `json:"system"`
	Os     MSSpeechConfigOs     `json:"os"`
	Device MSSpeechConfigDevice `json:"device"`
}

type MSSpeechConfig struct {
	Context MSSpeechConfigContext
}

func (ms *MicrosoftClient) GetConfigHeader(session string) []byte {
	requestId := "X-RequestId:" + session
	timestamp := "X-Timestamp:" + time.Now().Format(time.RFC3339)
	return []byte(configHeaderPath + timestamp + "\r\n" + configHeaderContentType + requestId + "\r\n\r\n")
}

func (ms *MicrosoftClient) sendSpeechConfig(session string) error {
	toSend := ms.GetConfigHeader(session)

	msDevice := MSSpeechConfigDevice{
		Manufacturer: "Anki",
		Model:        "computer",
		Version:      "1.0.0",
	}

	msOs := MSSpeechConfigOs{
		Platform: "Apple",
		Name:     "macOS",
		Version:  "10.12.6",
	}

	msSystem := MSSpeechConfigSystem{
		Version: "1.0.0",
	}

	msContext := MSSpeechConfigContext{
		System: msSystem,
		Os:     msOs,
		Device: msDevice,
	}

	msConfig := MSSpeechConfig{
		Context: msContext,
	}

	jsonBytes, err := json.Marshal(&msConfig)
	if err != nil {
		alog.Warn{"action": "marshal_config_json_string", "error": err}.Log()
		return err
	}

	toSend = append(toSend, jsonBytes...)

	err = ms.conn.WriteMessage(websocket.TextMessage, toSend)
	return nil
}

type LuisClient struct {
	appId   string
	luisKey string
}

type LuisResponseEntityResolution struct {
	Unit   *string        `json:"unit,omitempty"`
	Value  *string        `json:"value,omitempty"`
	Values *[]interface{} `json:"values,omitempty"`
}

type LuisResponseEntity struct {
	Entity     string                        `json:"entity"`
	Type       string                        `json:"type"`
	StartIndex int64                         `json:"startIndex"`
	EndIndex   int64                         `json:"endIndex"`
	Score      *float64                      `json:"score,omitempty"`
	Resolution *LuisResponseEntityResolution `json:"resolution,omitempty"`
}

type LuisResponseIntent struct {
	Intent string  `json:"intent"`
	Score  float64 `json:"score"`
}

type LuisResponse struct {
	Query            string               `json:"query"`
	TopScoringIntent LuisResponseIntent   `json:"topScoringIntent"`
	Entities         []LuisResponseEntity `json:"entities"`
}

type LuisResult struct {
	Transcript string
	Intent     string
	Score      float32
	Entities   string
	Response   LuisResponse
	Blob       []byte // raw Luis API response blob
}

func NewLuisClient(luisKey, appId string) *LuisClient {
	return &LuisClient{
		luisKey: luisKey,
		appId:   appId,
	}
}
func (lc *LuisClient) GetIntent(hypothesis string) (*LuisResult, error) {

	body, err := json.Marshal(hypothesis)
	if err != nil {
		alog.Warn{"action": "marshal_hypothesis_string", "error": err}.Log()
		return nil, err
	}

	req, err := http.NewRequest("POST", luiBaseUrl+lc.appId, bytes.NewBuffer(body))
	if err != nil {
		alog.Warn{"action": "create_luis_request", "error": err}.Log()
		return nil, err
	}

	req.Header.Add("Content-Type", "application/json")
	req.Header.Add(tokenRequestHeader, lc.luisKey)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		alog.Warn{"action": "create_luis_http_client", "status": resp.StatusCode, "error": err}.Log()
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		alog.Warn{"action": "get_luis_response", "status": resp.StatusCode}.Log()
		return nil, err
	}

	responseBlob, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	incoming := LuisResponse{}
	if err = json.Unmarshal(responseBlob, &incoming); err != nil {
		alog.Warn{"action": "decode_luis_response", "status": resp.StatusCode, "error": err}.Log()
		return nil, err
	}

	// debug stuff
	//tmp, _ := json.MarshalIndent(incoming, "", "    ")
	//fmt.Println(string(tmp))

	intent := luisFallbackIntent
	if incoming.TopScoringIntent.Score >= microsoftMinimumThreshold {
		intent = incoming.TopScoringIntent.Intent
	}

	alog.Info{"action": "get_luis_intent", "hypothesis": hypothesis, "intent": intent}.Log()
	result := &LuisResult{
		Transcript: hypothesis,
		Intent:     intent,
		Score:      float32(incoming.TopScoringIntent.Score),
		Entities:   entitiesToString(incoming.Entities),
		Response:   incoming,
		Blob:       responseBlob,
	}

	return result, nil
}

func entitiesToString(input interface{}) string {
	entitiesMap, err := cutil.FlattenInterfaceToStringMap("", input)
	if err != nil {
		alog.Warn{"action": "flatten_entity", "status": "unexpected_error", "error": err}.Log()
		return ""
	}

	var keys []string
	for k := range entitiesMap {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	output := ""
	for _, key := range keys {
		output = output + key + ":" + entitiesMap[key] + ","
	}
	return output
}
