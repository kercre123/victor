package sqs

import (
	"fmt"
	"testing"
	"time"
	"regexp"
	
	"github.com/anki/sai-go-accounts/db"	
	"github.com/anki/sai-go-accounts/db/dbtest"
	"github.com/anki/sai-go-accounts/models/user"
	"github.com/anki/sai-go-accounts/util"
	"github.com/stretchr/testify/suite"
)

type QResponse struct {
	resp []*SQSMessage
	err  error
}

type FakeQReader struct {
	responses <-chan QResponse
}

type SQSReaderSuite struct {
	dbtest.DBSuite
}

func TestSQSReaderSuite(t *testing.T) {
	suite.Run(t, new(SQSReaderSuite))
}

func (suite *SQSReaderSuite) SetupTest() {
	suite.DBSuite.SetupTest()
}

func (fr *FakeQReader) ReceiveMessage() ([]*SQSMessage, error) {
	r := <-fr.responses
	return r.resp, r.err
}

func (fr *FakeQReader) DeleteMessage(string) (error) {
	return nil
}

func createNamedModelUa(t *testing.T, username string, remoteId string) *user.UserAccount {
	ua := &user.UserAccount{
		NewPassword: util.Pstring("hashme"),
		User: &user.User{
			Status:    user.AccountActive,
			Username:  util.Pstring(username),
			GivenName: util.Pstring("given name"),
			Email:     util.Pstring("a@b"),
			EmailLang: util.Pstring("en-US"),
		},
	}
	return ua
}

func createFromUa(t *testing.T, ua *user.UserAccount) *user.UserAccount {
	errs := user.CreateUserAccount(ua)
	if errs != nil {
		t.Fatalf("Failed to create user %q account: %s", util.NS(ua.User.Username), errs)
	}
	return ua
}

func createNamedModelUser(t *testing.T, username string, remoteId string) *user.UserAccount {
	ua := createNamedModelUa(t, username, remoteId)
	createFromUa(t, ua)
	return ua
}

// GorpLogger interface
type ChanLogger struct {
	trace chan string
}

func (cl ChanLogger) Printf(format string, v ...interface{}) {
	matched,_ := regexp.MatchString(`update .users.`, fmt.Sprintf(format, v))
	
	if matched {
		cl.trace <- fmt.Sprintf(format, v)
	}
}

func (suite *SQSReaderSuite) TestSetAutodelete() {
	// setup test suite (db init)
	t := suite.T()

	// create a test user
	ua := createNamedModelUser(t, "user1", "")

	// setup a fake SQS queue
	sqsReadRetryInterval = time.Microsecond
	ch := make(chan QResponse, 4)
	responder := &FakeQReader{responses: ch}

	// setup a logger for gorp to grab just the sql update
	logger := ChanLogger{trace: make(chan string)}
	db.Dbmap.TraceOn("", logger)
	
	// send a message to the fake SQS queue
	ch <- QResponse{[]*SQSMessage{&SQSMessage{Body: `{"user_id":"`+ua.User.Id+`","order_id":"100001014"}`}}, nil}
	
	// setup the SQSReader
	sr := QueueReader{
		Reader: responder,
	}
	sr.Start(1)
	
	select {
	case sql := <-logger.trace:
		// no_autodelete is the 31st bound variable...
		matched, _ := regexp.MatchString("31:true", sql)
		if !matched {
			t.Fatalf("SQL update incorrect")
		}
	case <-time.After(time.Second * 5):
		t.Fatalf("SQL update not received")
	}

	// wait for the NoAutodelete flag to get set
 	timeoutChan := time.NewTimer(time.Second * 5).C
 	ticker := time.NewTicker(time.Millisecond * 250)

	for {
		select {
		case <- ticker.C:
			// fetch the user and check flag
			ua2, _ := user.AccountById(ua.User.Id)
			
			if(!ua2.User.NoAutodelete) {
				continue
			}
		case <- timeoutChan:
			t.Fatal("timed out waiting for NoAutodelete to be set")
		}
		
		ticker.Stop()
		break
	}
}
