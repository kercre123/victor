package postmarkapp

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"net/mail"
	"net/url"
	"os"
	"reflect"
	"strconv"
	"strings"
	"testing"

	"github.com/kr/pretty"
)

func createMessage() *Message {
	return &Message{
		From: &mail.Address{
			Name:    "Anki Test",
			Address: "accountmail@anki.com",
		},
		To: []*mail.Address{
			{
				Name:    "Tom Eliaz",
				Address: "tom@anki.com",
			},
		},
		Subject:  "TEST SUBJECT",
		TextBody: strings.NewReader("THIS IS MY MESSAGE"),
	}
} // end createMessage

func createExampleComMessage() *Message {
	return &Message{
		From: &mail.Address{
			Name:    "Anki Test",
			Address: "accountmail@anki.com",
		},
		To: []*mail.Address{
			{
				Name:    "Tom Eliaz",
				Address: "tom@example.com",
			},
		},
		Subject:  "TEST SUBJECT",
		TextBody: strings.NewReader("THIS IS MY MESSAGE"),
	}
}

var emailTemplateData = map[string]string{
	"Username": "Bob",
	"ToEmail":  "tom@anki.com",
	"URL":      "http://anki.com",
}

func createTemplateMessage() *TemplateMessage {

	return &TemplateMessage{
		From: &mail.Address{
			Name:    "Anki Test",
			Address: "accountmail@anki.com",
		},
		To: []*mail.Address{
			{
				Name:    "Tom Eliaz",
				Address: "tom@anki.com",
			},
		},
		Tag:           "tag",
		TemplateId:    100,
		TemplateModel: emailTemplateData,
	}
} // end createMessage

func TestEncodingTemplateMessage(t *testing.T) {
	msg := createTemplateMessage()
	buf := bytes.Buffer{}
	err := json.NewEncoder(&buf).Encode(msg)
	if err != nil {
		t.Error("Fail to encode template message")
		t.FailNow()
	}

	var f interface{}
	if err := json.Unmarshal(buf.Bytes(), &f); err != nil {
		t.Error("Expected valid json, got ", err)
		t.FailNow()
	}
	m := f.(map[string]interface{})

	b := make(map[string]string, len(m["TemplateModel"].(map[string]interface{})))
	for k, v := range m["TemplateModel"].(map[string]interface{}) {
		b[k] = v.(string)
	}

	if !reflect.DeepEqual(b, emailTemplateData) {
		t.Error("Expected deep equals ", b, " ", emailTemplateData)
	}
}

func TestAgainstPostmarkApp(t *testing.T) {
	if os.Getenv("WITH_NETWORK") == "" {
		t.Skip("skipping test that requires network connection, set WITH_NETWORK to run test.")
	}

	c := Client{
		ApiKey: "POSTMARK_API_TEST",
	}

	res, err := c.Send(createMessage())
	if err != nil {
		t.Error("Expected valid reply from API, got ", err)
		t.FailNow()
	}

	if res.ErrorCode != 0 {
		fmt.Printf("%#v\n", res)
		t.Error("Expected 0 error code, got ", res)
	}
} // end TestAgainstPostmarkApi

func TestBadAPIKEY(t *testing.T) {
	if os.Getenv("WITH_NETWORK") == "" {
		t.Skip("skipping test that requires network connection, set WITH_NETWORK to run test.")
	}

	c := Client{
		ApiKey: "POSTMARK_API_BADKEY",
	}

	res, err := c.Send(createMessage())
	if err != nil {
		t.Error("Expected valid reply from API, got ", err)
		t.FailNow()
	}

	if res.ErrorCode == 0 {
		fmt.Printf("%#v\n", res)
		t.Error("Expected fail error code, got ", res)
	}
} // end TestBadAPIKey

func TestBadToEmail(t *testing.T) {
	c := Client{
		ApiKey: "POSTMARK_API_TEST",
	}

	m := createMessage()
	m.To[0].Address = "TEST"

	res, err := c.Send(m)
	if err != nil {
		t.Error("Expected valid reply from API, got ", err)
		t.FailNow()
	}

	if res.ErrorCode == 0 {
		fmt.Printf("%#v\n", res)
		t.Error("Expected fail error code, got ", res)
	}
}

func TestExampleCom(t *testing.T) {
	if os.Getenv("WITH_NETWORK") == "" {
		t.Skip("skipping test that requires network connection, set WITH_NETWORK to run test.")
	}

	c := Client{
		ApiKey: "POSTMARK_API_TEST",
	}

	res, err := c.Send(createMessage())
	if err != nil {
		t.Error("Expected valid reply from API, got ", err)
		t.FailNow()
	}

	if res.ErrorCode != 0 {
		fmt.Printf("%#v\n", res)
		t.Error("Expected 0 error code, got ", res)
	}
	if res.Message != "Skipped EXAMPLE.COM" {
		t.Error("Expected message to be skipped due to example.com")
	}
} // end TestAgainstPostmarkApi

type pmResp struct {
	ErrorCode   int
	Message     string
	MessageID   string
	SubmittedAt string
	To          string
}

func TestBatchSendOK(t *testing.T) {
	var recvmsgs []struct{ To string }
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		err := json.NewDecoder(r.Body).Decode(&recvmsgs)
		if err != nil {
			fmt.Println("Failed to decode JSON body", err)
			return
		}
		var resp []Result
		for i, msg := range recvmsgs {
			resp = append(resp, Result{
				ErrorCode:   0,
				Message:     "OK",
				MessageID:   strconv.Itoa(i),
				SubmittedAt: "2010-11-26T12:01:05.1794748-05:00",
				To:          msg.To,
			})
		}
		json.NewEncoder(w).Encode(resp)
	}))
	defer ts.Close()

	u, _ := url.Parse(ts.URL)
	c := Client{
		ApiKey:     "POSTMARK_API_TESET",
		Host:       u.Host,
		HostScheme: "http",
	}

	msg1 := createMessage()
	msg1.To[0].Address = "tom+one@anki.com"
	msg2 := createExampleComMessage()
	msg3 := createMessage()
	msg3.To[0].Address = "tom+two@anki.com"
	msgs := []*Message{msg1, msg2, msg3}

	results, err := c.SendBatch(msgs)
	if err != nil {
		t.Fatal("Unexpected error", err)
	}

	expectedResults := []*Result{
		{
			ErrorCode:   0,
			Message:     "OK",
			MessageID:   "0",
			SubmittedAt: "2010-11-26T12:01:05.1794748-05:00",
			To:          `"Tom Eliaz" <tom+one@anki.com>`,
		}, {
			Message: "Skipped EXAMPLE.COM",
		}, {
			ErrorCode:   0,
			Message:     "OK",
			MessageID:   "1",
			SubmittedAt: "2010-11-26T12:01:05.1794748-05:00",
			To:          `"Tom Eliaz" <tom+two@anki.com>`,
		},
	}

	if !reflect.DeepEqual(results, expectedResults) {
		pretty.Println("EXPECTED", expectedResults)
		pretty.Println("ACTUAL  ", results)
		t.Errorf("Incorrect results")
	}

	expRecvmsgs := []struct{ To string }{
		{`"Tom Eliaz" <tom+one@anki.com>`},
		{`"Tom Eliaz" <tom+two@anki.com>`}}
	if !reflect.DeepEqual(recvmsgs, expRecvmsgs) {
		pretty.Println("recvmsgs", recvmsgs)
		t.Errorf("Incorrect params passed to postmark API")
	}
}
