package main

import (
	"flag"
	"fmt"
	"os"
	"strings"

	_ "golang.org/x/net/trace"

	"github.com/anki/sai-go-util/log"

	"github.com/anki/sai-go-util/strutil"

	tc "github.com/anki/sai-chipper-voice/client/testclient"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"

	"context"
	"encoding/json"
	"time"
)

const (
	defaultServer = "127.0.0.1"
	defaultPort   = 10000
)

var (
	addr      = flag.String("addr", defaultServer, "The server address in the format of host:port")
	port      = flag.Int("port", defaultPort, "The server port")
	certFile  = flag.String("cert-file", "", "The TLS cert file for local testing")
	svc       = flag.String("svc", "default", "intent service to us: default, goog or ms")
	audioFile = flag.String("audio", "", "Name of audio file to stream")
	noiseFile = flag.String("noise", "", "Name of noise file to stream after audio file")
)

const (
	setTimerTextHour    = "set timer for 1 hour 20 minutes and 30 seconds"
	setTimerTextMin     = "set timer for 3 minutes"
	weatherForecastText = "what is the weather for tomorrow"
	weatherRemoteText   = "what is the weather in Paris Texas"
	behaviorText        = "do a fist bump"
	nameText            = "my name is Jack"
	doTrickText         = "do a trick"
	statusText          = "How are you"
	deletePhotoText     = "delete the photo"
	stopTimerText       = "turn off the timer"
	playMessageText     = "play all messages for tom tom"
	recordMessageText   = "record a message for Frankie"
	takePhotoText       = "take a photo of me"
	helpText            = "help me with timer"
)

type Param struct {
	key    string
	value  string
	intent string
}

func testCases() map[string]Param {
	testCases := make(map[string]Param)
	testCases[setTimerTextHour] = Param{"timer_duration", "4830", "intent_clock_settimer_extend"}
	testCases[setTimerTextMin] = Param{"timer_duration", "180", "intent_clock_settimer_extend"}
	testCases[stopTimerText] = Param{"entity_behavior_stoppable", "timer", "intent_global_stop_extend"}

	testCases[weatherForecastText] = Param{"is_forecast", "true", "intent_weather_extend"}
	testCases[weatherRemoteText] = Param{"speakable_location_string", "paris", "intent_weather_extend"}

	testCases[behaviorText] = Param{"entity_behavior", "fist_bump", "intent_play_specific_extend"}
	testCases[doTrickText] = Param{"", "", "intent_play_anytrick"}

	testCases[nameText] = Param{"username", "jack", "intent_names_username_extend"}

	testCases[statusText] = Param{"", "", "intent_greeting_hello"}

	testCases[deletePhotoText] = Param{"entity_behavior_deletable", "photo", "intent_global_delete_extend"}

	testCases[playMessageText] = Param{"given_name", "tom tom", "intent_message_playmessage_extend"}
	testCases[recordMessageText] = Param{"given_name", "frankie", "intent_message_recordmessage_extend"}

	testCases[takePhotoText] = Param{"entity_photo_selfie", "photo_selfie", "intent_photo_take_extend"}

	testCases[helpText] = Param{"entity_topic", "set_timer", "intent_system_discovery_extend"}
	return testCases
}

func TestParameters(client *tc.ChipperClient) []string {
	testCases := testCases()

	var errorIntents []string
	for phrase, expected := range testCases {
		result := client.TextIntent(phrase)

		if result.Action != expected.intent {
			errorIntents = append(errorIntents, result.Action)
			alog.Error{"action": "test_params", "error": "wrong_intent",
				"expect": expected.intent, "get": result.Action}.Log()
			continue
		}

		value, ok := result.Parameters[expected.key]
		if !ok && expected.key != "" {
			errorIntents = append(errorIntents, result.Action)
			alog.Error{"action": "test_params", "intent": result.Action,
				"error": "missing_param_key", "expect": expected.key, "get": result.Parameters}.Log()
			continue
		}

		if expected.value != "" && !strings.Contains(strings.ToLower(value), expected.value) {
			errorIntents = append(errorIntents, result.Action)
			alog.Error{"action": "test_params", "intent": result.Action,
				"error": "wrong_value", "expect": expected.value, "get": value}.Log()
			continue
		}
	}

	return errorIntents
}

// text intents to test parameters and intent correctness
// requires chipper server to be running locally or in the cloud
func main() {
	alog.ToStdout()

	flag.Parse()

	session, err := strutil.ShortLowerUUID()
	if err != nil {
		alog.Error{"action": "create_session_id", "status": "fail", "error": err}.Log()
		os.Exit(1)
	}

	ctx, _ := context.WithTimeout(context.Background(), 20*time.Second)
	cert := ""
	if certFile != nil {
		cert = *certFile
	}

	client, err := tc.NewTestClient(ctx, *addr, *port, cert,
		tc.WithDeviceId("integration-test"),
		tc.WithSession(session),
		tc.WithAudioEncoding(tc.Encoding["opus"]),
		tc.WithLangCode(pb.LanguageCode_ENGLISH_US),
		tc.WithService(tc.Services[*svc]),
		tc.WithSingleUtterance(true),
	)
	if err != nil {
		os.Exit(1)
	}
	defer client.Conn.Close()

	// Test entities parsing
	testErrors := TestParameters(client)
	fmt.Println("Parameters parsing errors: ", len(testErrors))

	// stream audio
	d, err := tc.ReadFiles(*noiseFile, *audioFile)
	if err != nil {
		alog.Error{"action": "read_files", "error": err}.Log()
		os.Exit(1)
	}

	client.SetMode(tc.Modes["command"])
	client.SetContext(ctx)
	res, _ := client.StreamFromFile(d)
	if res != nil {
		ejson, _ := json.MarshalIndent(res, "", "    ")
		fmt.Println("Audiofile ", *audioFile)
		fmt.Println(string(ejson))
	}

}
