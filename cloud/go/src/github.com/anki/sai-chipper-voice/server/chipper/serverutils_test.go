package chipper

import (
	"context"
	"strconv"
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc"

	tms "github.com/anki/sai-token-service/model"
)

//
// Test _extend for intent-name
//

var intentNameTests = []struct {
	intent string
	extend bool
}{
	{"intent_clock_settimer", false},
	{"intent_clock_settimer_extend", true},
	{"intent_names_username", false},
	{"intent_names_username_extend", true},
	{"intent_weather", false},
	{"intent_weather_extend", true},
	{"intent_play_specific", false},
	{"intent_play_specific_extend", true},
	{"intent_message_playmessage", false},
	{"intent_message_playmessage_extend", true},
	{"intent_message_recordmessage", false},
	{"intent_message_recordmessage_extend", true},
	{"intent_global_stop", false},
	{"intent_global_stop_extend", true},
	{"intent_global_delete", false},
	{"intent_global_delete_extend", true},
	{"intent_photo_take", false},
	{"intent_photo_take_extend", true},
	{"intent_system_discovery", false},
	{"intent_system_discovery_extend", true},
	{"_extend", true},
	{"intent_extend", true},
	{"intent_extend_", false},
	{"intent_extend_whatever", false},
	{"extend_extend", true},
}

func TestIntentNameExtend(t *testing.T) {
	for _, tst := range intentNameTests {
		result := ExtendIntent(tst.intent)
		if result != tst.extend {
			t.Errorf("test=intent-name-extend, intent_name=%#v, expects=%#v, actual=%#v",
				tst.intent, tst.extend, result)
		}
	}
}

func TestCreateSessionId(t *testing.T) {
	device := "testdevice"
	session := "testsession"
	id := createSessionId(device, session)
	assert.Equal(t, "testdevice-testsession", id)
}

func TestFlushAudioWithData(t *testing.T) {
	nChan := 10
	audioChan := make(chan []byte, nChan)
	nBytes := 0
	for i := 0; i < nChan; i++ {
		intBytes := []byte(strconv.Itoa(i))
		nBytes += len(intBytes)
		audioChan <- intBytes
	}

	req := chipperRequest{
		device:  "test",
		session: "testing",
	}

	b := flushAudio(audioChan, req, "TEST")
	assert.Equal(t, nBytes, len(b.Bytes()))
}

func TestFlushAudioNoData(t *testing.T) {
	nChan := 10
	audioChan := make(chan []byte, nChan)
	req := chipperRequest{
		device:  "test",
		session: "testing",
	}

	b := flushAudio(audioChan, req, "TEST")
	assert.Equal(t, 0, len(b.Bytes()))
}

type deviceIdTest struct {
	ctx      context.Context
	pbDevice string
	expected string
	isESN    bool
	err      error
}

func getTokenCtx(id string) context.Context {
	token := tms.Token{
		RequestorId: id,
	}
	ctx := context.Background()
	return tms.WithAccessToken(ctx, &token)
}

func TestGetDeviceId(t *testing.T) {

	testCases := []deviceIdTest{
		// these check for device id in the token
		{getTokenCtx("vic:00e20119"), "", "00e20119", true, nil},
		{getTokenCtx("vic:00500193"), "", "00500193", true, nil},
		{getTokenCtx("vic:00e20130"), "", "00e20130", true, nil},
		{getTokenCtx("vic:00e2019f"), "", "00e2019f", true, nil},
		{getTokenCtx("vic:003002a1"), "", "003002a1", true, nil},
		{getTokenCtx("vic:00e20175"), "", "00e20175", true, nil},
		{getTokenCtx("vic:00e2017a"), "", "00e2017a", true, nil},
		{getTokenCtx("vic:10e20119"), "", "10e20119", true, nil},
		{getTokenCtx("vic:fff20119"), "", "fff20119", true, nil},
		{getTokenCtx("vic:00e2017aaa"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx("vic:00e20!7?"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx("vic:00e20!7?"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx("vic:00e2017g"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx("vic:king-+.shy.@anki.com:0"), "", "king-+.shy.@anki.com", false, nil},
		{getTokenCtx("vic:samba+test@anki.com:0"), "", "samba+test@anki.com", false, nil},
		{getTokenCtx("vic:samba+test@anki.com:5"), "", "samba+test@anki.com", false, nil},
		{getTokenCtx("vic:samba+test@anki.com:29"), "", "samba+test@anki.com", false, nil},
		{getTokenCtx("vic:samba+test@anki.com:1234567"), "", "samba+test@anki.com", false, nil},
		{getTokenCtx("vic:samba+test@anki.com:ABC567"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx("vic:samba+test@anki.com"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx("vic:test@gmail.com:0"), "", "", false, ErrorInvalidRequestorFormat},
		{getTokenCtx(""), "", "", false, ErrorEmptyRequestorId},
		{getTokenCtx(""), "", "", false, ErrorEmptyRequestorId},

		// these have no token, will get device id from the pb field
		{context.Background(), "12345678", TestDeviceIdPrefix + "_" + "12345678", false, nil},
		{context.Background(), "12345678000", TestDeviceIdPrefix + "_" + "12345678", false, nil},
		{context.Background(), "123456bcd", TestDeviceIdPrefix + "_" + "123456bc", false, nil},
		{context.Background(), "123456", "", false, ErrorInvalidPbDeviceId},
		{context.Background(), "", "", false, ErrorInvalidPbDeviceId},
	}
	for i, tc := range testCases {
		t.Log(i)

		id, isESN, err := getDeviceId(tc.ctx, tc.pbDevice)
		if tc.err == nil {
			assert.NoError(t, err)
		} else {
			assert.Equal(t, grpc.Code(err), grpc.Code(tc.err))
		}
		assert.Equal(t, tc.expected, id)
		assert.Equal(t, tc.isESN, isESN)
	}

}
