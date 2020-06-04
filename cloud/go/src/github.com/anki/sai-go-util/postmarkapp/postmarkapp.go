// Anki's homed version of the postmarkapp go API.
// Slight modifications to the version found at github.com/hjr265/postmark.go/postmark to make use of the DefaultHost (and thus allow compilation).
package postmarkapp

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"errors"
	"io"
	"io/ioutil"
	"net/http"
	"net/mail"
	"net/url"
	"strings"
	"time"

	"github.com/anki/sai-go-util/http/metrics/metricsclient"
	"github.com/anki/sai-go-util/metricsutil"
)

// DefaultHost is the PostmarkApp API host, which should not change
const (
	DefaultHost    = "api.postmarkapp.com"
	BatchSendLimit = 500 // Postmark supports up to 500 messages per batch api call
)

var (
	registry  = metricsutil.NewRegistry("postmark")
	client, _ = metricsclient.New("postmark")
)

// Message is the email message we'll build to send.
// You can specify either/both Text and HTML body.
// You must use a From email that has been validated by postmarkapp through their web interface.
type Message struct {
	From        *mail.Address
	To          []*mail.Address
	Cc          []*mail.Address
	Bcc         []*mail.Address
	Subject     string
	Tag         string
	HtmlBody    io.Reader
	TextBody    io.Reader
	ReplyTo     *mail.Address
	Headers     mail.Header
	Attachments []Attachment
}

func (m *Message) MarshalJSON() ([]byte, error) {
	doc := &struct {
		From        string
		To          string
		Cc          string
		Bcc         string
		Subject     string
		Tag         string
		HtmlBody    string
		TextBody    string
		ReplyTo     string
		Headers     []map[string]string
		Attachments []Attachment `json:",omitempty"`
	}{}

	doc.From = m.From.String()
	to := []string{}
	for _, addr := range m.To {
		to = append(to, addr.String())
	}
	doc.To = strings.Join(to, ", ")
	cc := []string{}
	for _, addr := range m.Cc {
		cc = append(cc, addr.String())
	}
	doc.Cc = strings.Join(cc, ", ")
	bcc := []string{}
	for _, addr := range m.Bcc {
		bcc = append(bcc, addr.String())
	}
	doc.Bcc = strings.Join(bcc, ", ")
	doc.Subject = m.Subject
	doc.Tag = m.Tag
	if m.HtmlBody != nil {
		htmlBody, err := ioutil.ReadAll(m.HtmlBody)
		if err != nil {
			return nil, err
		}
		doc.HtmlBody = string(htmlBody)
	}
	if m.TextBody != nil {
		textBody, err := ioutil.ReadAll(m.TextBody)
		if err != nil {
			return nil, err
		}
		doc.TextBody = string(textBody)
	}
	if m.ReplyTo != nil {
		doc.ReplyTo = m.ReplyTo.String()
	}
	headers := []map[string]string{}
	for k, vs := range m.Headers {
		for _, v := range vs {
			headers = append(headers, map[string]string{
				"Name":  k,
				"Value": v,
			})
		}
	}
	doc.Headers = headers
	doc.Attachments = m.Attachments

	return json.Marshal(doc)
}

// Client is your way of interacting with the API using Send.
// For the ApiKey there are three possible values.
// If you want to bypass email sending and not test, set the ApiKey to NO_EMAIL, it will return true, nil from Send()
// If you want to test your email against the PostmarkAPI, but never actually send it, set the key to POSTMARK_API_TEST
// If you want to send real emails, set it to your postmark API key.
//
// Host should rarely if ever be set.
type Client struct {
	ApiKey            string        // See documentation about possible values
	Host              string        // Host for the API endpoints, DefaultHost if ""
	HostScheme        string        // "http" or "https", defaults to "https"
	LastEmail         chan *Message // When StoreLastEmail, will be the last email message
	LastEmailTemplate chan *TemplateMessage
	LastBatchEmail    chan []*Message
}

// TemplateMessage is the email message used to send via Template.
// This does not validate the template exists.
// We omit attachments as we have never needed that functionality
type TemplateMessage struct {
	TemplateId    int
	TemplateModel interface{} // Data object for the template
	From          *mail.Address
	To            []*mail.Address
	Cc            []*mail.Address
	Bcc           []*mail.Address
	Tag           string
	ReplyTo       *mail.Address
	Headers       mail.Header
}

func (m *TemplateMessage) MarshalJSON() ([]byte, error) {
	doc := &struct {
		TemplateId    int
		TemplateModel interface{}
		From          string
		To            string
		Cc            string
		Bcc           string
		Tag           string
		ReplyTo       string
		Headers       []map[string]string
	}{}

	doc.TemplateId = m.TemplateId
	doc.TemplateModel = m.TemplateModel
	doc.From = m.From.String()
	to := []string{}
	for _, addr := range m.To {
		to = append(to, addr.String())
	}
	doc.To = strings.Join(to, ", ")
	cc := []string{}
	for _, addr := range m.Cc {
		cc = append(cc, addr.String())
	}
	doc.Cc = strings.Join(cc, ", ")
	bcc := []string{}
	for _, addr := range m.Bcc {
		bcc = append(bcc, addr.String())
	}
	doc.Bcc = strings.Join(bcc, ", ")
	doc.Tag = m.Tag
	if m.ReplyTo != nil {
		doc.ReplyTo = m.ReplyTo.String()
	}
	headers := []map[string]string{}
	for k, vs := range m.Headers {
		for _, v := range vs {
			headers = append(headers, map[string]string{
				"Name":  k,
				"Value": v,
			})
		}
	}
	doc.Headers = headers

	return json.Marshal(doc)
}

type Attachment struct {
	Name        string
	Content     io.Reader
	ContentType string
}

func (a *Attachment) MarshalJSON() ([]byte, error) {
	doc := &struct {
		Name        string
		Content     string
		ContentType string
	}{}

	doc.Name = a.Name
	content, err := ioutil.ReadAll(a.Content)
	if err != nil {
		return nil, err
	}
	doc.Content = base64.StdEncoding.EncodeToString(content)
	doc.ContentType = a.ContentType

	return json.Marshal(doc)
}

func (c *Client) endpoint(path string) *url.URL {
	url := &url.URL{}
	if c.HostScheme == "" {
		url.Scheme = "https"
	} else {
		url.Scheme = c.HostScheme
	}

	if c.Host == "" {
		url.Host = DefaultHost
	} else {
		url.Host = c.Host
	}

	url.Path = path

	return url
}

var (
	postmarkSendTimer = metricsutil.GetTimer("Send", registry)
)

// Send sends a single message.
// This method returns the Result structure. Any issues other than network access to the API
// will result in an Reply structure that has a non-zero ErrorCode.
// You will want to check for err from this method as well as an ErrorCode reply. See tests.
// If you set LastEmail to a valid chan, we'll put the msg on that chan rather than send it.
// Messages to *#example.com or where the ApiKey is NO_EMAIL are dropped
func (c *Client) Send(msg *Message) (*Result, error) {
	if c.LastEmail != nil {
		c.LastEmail <- msg
		return &Result{Message: "Skipped ON CHAN"}, nil
	}
	if c.ApiKey == "NO_EMAIL" {
		return &Result{Message: "Skipped NO_EMAIL"}, nil
	}
	if msg.To[0] != nil && strings.HasSuffix(strings.ToLower(msg.To[0].Address), "@example.com") {
		return &Result{Message: "Skipped EXAMPLE.COM"}, nil
	}

	buf := bytes.Buffer{}
	err := json.NewEncoder(&buf).Encode(msg)
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequest("POST", c.endpoint("email").String(), &buf)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("X-Postmark-Server-Token", c.ApiKey)

	start := time.Now()
	defer postmarkSendTimer.UpdateSince(start)
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}

	defer resp.Body.Close()
	res := &Result{}
	json.NewDecoder(resp.Body).Decode(res)
	return res, nil
}

var (
	postmarkSendTemplateTimer = metricsutil.GetTimer("SendTemplate", registry)
)

// SendTemplate sends a single message using a template.
// This method returns the Result structure. Any issues other than network access to the API
// will result in an Reply structure that has a non-zero ErrorCode.
// You will want to check for err from this method as well as an ErrorCode reply. See tests.
// If you set LastEmail to a valid chan, we'll put the msg on that chan rather than send it.
// Messages to *#example.com or where the ApiKey is NO_EMAIL are dropped
func (c *Client) SendTemplate(msg *TemplateMessage) (*Result, error) {
	if c.LastEmailTemplate != nil {
		c.LastEmailTemplate <- msg
		return &Result{Message: "Skipped ON CHAN"}, nil
	}
	if c.ApiKey == "NO_EMAIL" {
		return &Result{Message: "Skipped NO_EMAIL"}, nil
	}
	if msg.To[0] != nil && strings.HasSuffix(strings.ToLower(msg.To[0].Address), "@example.com") {
		return &Result{Message: "Skipped EXAMPLE.COM"}, nil
	}

	buf := bytes.Buffer{}
	err := json.NewEncoder(&buf).Encode(msg)
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequest("POST", c.endpoint("email/withTemplate").String(), &buf)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("X-Postmark-Server-Token", c.ApiKey)

	start := time.Now()
	defer postmarkSendTemplateTimer.UpdateSince(start)
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}

	defer resp.Body.Close()
	res := &Result{}
	json.NewDecoder(resp.Body).Decode(res)
	return res, nil
}

// Result encodes the values returned from the postmarkapp API call.
// It's essential to always check for ErrorCode == 0.
type Result struct {
	ErrorCode   int
	Message     string
	MessageID   string
	SubmittedAt string
	To          string
}

var (
	postmarkSendBatchTimer = metricsutil.GetTimer("SendBatch", registry)
	sendBatchOverLimit     = metricsutil.GetCounter("SendBatchOverLimit", registry)
)

// SendBatch sends multiple messages using the batch API.
// Like Send, you will have to iterate the Reply structures to determine if messages made it
// out of postmarkapp.
func (c *Client) SendBatch(msgs []*Message) (results []*Result, err error) {
	results = make([]*Result, len(msgs))

	if len(msgs) > BatchSendLimit {
		sendBatchOverLimit.Inc(1)
		return nil, errors.New("Too many messages for batch send call")
	}

	if c.LastBatchEmail != nil {
		c.LastBatchEmail <- msgs
		for i, m := range msgs {
			results[i] = &Result{Message: "Skipped ON CHAN", To: m.To[0].Address}
		}
		return results, nil
	}

	if c.ApiKey == "NO_EMAIL" {
		for i, m := range msgs {
			results[i] = &Result{Message: "Skipped NO_EMAIL", To: m.To[0].Address}
		}
		return results, nil
	}

	var j int
	msgmap := make(map[int]int)
	filtered := make([]*Message, 0, len(msgs))
	for i, m := range msgs {
		if m.To[0] != nil && !strings.HasSuffix(strings.ToLower(m.To[0].Address), "@example.com") {
			filtered = append(filtered, m)
			msgmap[j] = i
			j++
		} else {
			results[i] = &Result{Message: "Skipped EXAMPLE.COM"}
		}
	}

	buf := bytes.Buffer{}
	err = json.NewEncoder(&buf).Encode(filtered)
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequest("POST", c.endpoint("email/batch").String(), &buf)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("X-Postmark-Server-Token", c.ApiKey)

	start := time.Now()
	defer postmarkSendBatchTimer.UpdateSince(start)
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}

	res := []*Result{}
	json.NewDecoder(resp.Body).Decode(&res)

	for j, result := range res {
		results[msgmap[j]] = result
	}
	return results, nil
}
