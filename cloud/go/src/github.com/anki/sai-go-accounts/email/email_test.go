package email

import (
	"errors"
	"io/ioutil"
	"os"
	"path/filepath"
	"reflect"
	"strings"
	"testing"
	"time"

	"fmt"

	"github.com/anki/sai-go-accounts/util"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/postmarkapp"
	"github.com/kr/pretty"
)

func init() {
	alog.ToStdout()
}

const URL_ROOT = "https://test.anki.com"

var tmpldir string

var testTime = time.Date(2015, 12, 1, 12, 13, 14, 0, time.UTC)

func sendEmail(msg *AnkiEmail, apilocator string) (*postmarkapp.Message, error) {
	if apilocator == "" {
		apilocator = "dev"
	}
	pmclient := &postmarkapp.Client{
		LastEmail: make(chan *postmarkapp.Message, 1),
	}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: apilocator}
	if err := e.Send(msg); err != nil {
		return nil, err
	}
	select {
	case msg := <-pmclient.LastEmail:
		return msg, nil
	case <-time.After(time.Second):
		return nil, errors.New("Timed out waiting for message")
	}
}

func init() {
	if tmpldir = os.Getenv("TEMPLATE_DIR"); tmpldir == "" {
		tmpldir, _ = os.Getwd()
		tmpldir = filepath.Join(tmpldir, "templates")
	}
}

var verificationTests = []struct {
	name             string
	emailFunc        VerificationEmailFunc
	expectedTemplate string
}{
	{"verification", VerificationEmail, "Verification"},
	{"7-day-reminder", Verification7DayReminderEmail, "Verification7dReminder"},
	{"14-day-reminder", Verification14DayReminderEmail, "Verification14dReminder"},
}

func TestAnkiEmailer_VerificationEmail(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	for _, test := range verificationTests {
		for _, isAdult := range []bool{true, false} {
			msg := test.emailFunc(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", isAdult, "0000-00-00", "TOKEN", util.Pstring("en"), "overdrive", testTime}, nil)

			email, err := sendEmail(msg, "dev")

			if err != nil {
				t.Fatal("Error on send", err)
			}

			if email == nil {
				t.Fatal("No email created for Verification")
			}
			byt, _ := ioutil.ReadAll(email.TextBody)
			txt := string(byt)
			if !strings.Contains(txt, "https://test.anki.com/email-verifying?user_id=USERID&token=TOKEN&dloc=dev") {
				t.Error("Email does not contain the correct URL for validation ", txt)
			}
			expectedTemplate := test.expectedTemplate
			if !isAdult {
				expectedTemplate += "Under13"
			}
			if email.Headers["X-Anki-Tmpl"][0] != expectedTemplate {
				t.Errorf("test=%s isAdult=%t expectedTemplate=%q actual=%q", test.name, isAdult, expectedTemplate, email.Headers["X-Anki-Tmpl"][0])
			}
		}
	}
} // end VerifyAccount

func TestAnkiEmailer_VerificationEmailFrench(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	for _, test := range verificationTests {
		for _, isAdult := range []bool{true, false} {
			msg := test.emailFunc(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", isAdult, "0000-00-00", "TOKEN", util.Pstring("fr"), "overdrive", testTime}, nil)

			email, err := sendEmail(msg, "dev")

			if err != nil {
				t.Fatal("Error on send", err)
			}

			if email == nil {
				t.Fatal("No email created for Verification")
			}
			byt, _ := ioutil.ReadAll(email.TextBody)
			txt := string(byt)
			if !strings.Contains(txt, "https://test.anki.com/email-verifying?user_id=USERID&token=TOKEN&dloc=dev") {
				t.Error("Email does not contain fr canary ", txt)
			}
			expectedTemplate := test.expectedTemplate
			if !isAdult {
				expectedTemplate += "Under13"
			}
			if email.Headers["X-Anki-Tmpl"][0] != expectedTemplate {
				t.Errorf("test=%s isAdult=%t expectedTemplate=%q actual=%q", test.name, isAdult, expectedTemplate, email.Headers["X-Anki-Tmpl"][0])
			}
		}
	}
} // end VerifyAccountFrench

func TestAnkiEmailer_PostVerificationEmail(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	msg := PostVerificationEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAMETOM", "USERIDTOM", true, "0000-00-00", "TOKENTOM", util.Pstring("en"), "overdrive", testTime})
	email, err := sendEmail(msg, "dev")

	if err != nil {
		t.Fatal("Error on send", err)
	}

	if email == nil {
		t.Fatal("No email created for postVerification Test")
	}
	byt, _ := ioutil.ReadAll(email.TextBody)
	txt := string(byt)
	if !strings.Contains(txt, "Thank you for activating your") {
		t.Error("Email does not contain the correct child activation text ", txt)
	}
	if email.Headers["X-Anki-Tmpl"][0] != "PostVerificationUnder13" {
		t.Fatal("Email does not have needed header PostVerificationUnder13", email.Headers)
	}
}

func TestAnkiEmailer_ForgotPasswordEmail(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	msg := ForgotPasswordEmail(e, "Tom", "Eliaz", "tom@example.com", []AccountInfo{{"USERNAMETOM", "USERIDTOM", true, "0000-00-00", "TOKENTOM", util.Pstring("de"), "overdrive", testTime}, {"USERNAMEBOB", "USERIDBOB", true, "0000-00-00", "TOKENBOB", util.Pstring("en"), "overdrive", testTime}}, nil)

	email, err := sendEmail(msg, "dev")

	if err != nil {
		t.Fatal("Error on send", err)
	}

	if email == nil {
		t.Fatal("No email created for forgot password")
	}
	byt, _ := ioutil.ReadAll(email.TextBody)
	txt := string(byt)
	if !strings.Contains(txt, "https://test.anki.com/password-reset?user_id=USERIDBOB&token=TOKENBOB&dloc=") {
		t.Error("Email does not contain the correct URL for reset BOB ", txt)
	}
	if !strings.Contains(txt, "https://test.anki.com/password-reset?user_id=USERIDTOM&token=TOKENTOM&dloc=") {
		t.Error("Email does not contain the correct URL for reset TOM ", txt)
	}
	if !strings.Contains(txt, "Select the account you wish to reset the password for") {
		t.Error("Email does not contain the correct multi-user prompt ", txt)
	}
	if email.Headers["X-Anki-Tmpl"][0] != "ForgotPassword" {
		t.Fatal("Email does not have needed header ForgotPassword:", email.Headers)
	}
	if !strings.Contains(email.Subject, "Anki Account Password Reset Requested") {
		t.Error("Email subject is not in English: ", email.Subject)
	}
}

func TestAnkiEmailer_ForgotPasswordFrenchEmail(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	msg := ForgotPasswordEmail(e, "Tom", "Eliaz", "tom@example.com", []AccountInfo{{"USERNAMETOMF", "USERIDTOMF", true, "0000-00-00", "TOKENTOMF", util.Pstring("fr"), "overdrive", testTime}, {"USERNAMEBOBF", "USERIDBOBF", true, "0000-00-00", "TOKENBOBF", util.Pstring("fr"), "overdrive", testTime}}, nil)
	email, err := sendEmail(msg, "dev")

	if err != nil {
		t.Fatal("Error on send", err)
	}

	if email == nil {
		t.Fatal("No email created for forgot password")
	}
	byt, _ := ioutil.ReadAll(email.TextBody)
	txt := string(byt)
	if !strings.Contains(txt, "Demande de réinitialisation de mot de passe") {
		t.Error("Email does not contain the correct French Text ", txt)
	}
	if !strings.Contains(txt, "https://test.anki.com/password-reset?user_id=USERIDBOBF&token=TOKENBOBF&dloc=") {
		t.Error("Email does not contain the correct URL for reset BOB ", txt)
	}
	if !strings.Contains(txt, "https://test.anki.com/password-reset?user_id=USERIDTOMF&token=TOKENTOMF&dloc=") {
		t.Error("Email does not contain the correct URL for reset TOM ", txt)
	}
	if !strings.Contains(txt, "Sélectionnez le compte que vous souhaitez pour lequel vous voulez reinitialiser le mot de passe") {
		t.Error("Email does not contain the correct multi-user prompt ", txt)
	}
	if email.Headers["X-Anki-Tmpl"][0] != "ForgotPassword" {
		t.Fatal("Email does not have needed header ForgotPassword:", email.Headers)
	}
	if !strings.Contains(email.Subject, "Demande de réinitialisation de mot de passe.") {
			t.Error("Email subject is not in French: ", email.Subject)
	}
}

func TestAnkiEmailer_ForgotPasswordGermanEmail(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	msg := ForgotPasswordEmail(e, "Tom", "Eliaz", "tom@example.com", []AccountInfo{{"USERNAMETOM", "USERIDTOM", true, "0000-00-00", "TOKENTOM", util.Pstring("de"), "overdrive", testTime}, {"USERNAMEBOB", "USERIDBOB", true, "0000-00-00", "TOKENBOB", util.Pstring("de"), "overdrive", testTime}}, nil)
	email, err := sendEmail(msg, "dev")

	if err != nil {
		t.Fatal("Error on send", err)
	}

	if email == nil {
		t.Fatal("No email created for forgot password")
	}
	byt, _ := ioutil.ReadAll(email.TextBody)
	txt := string(byt)
	if !strings.Contains(txt, "DU HAST DARUM") {
		t.Error("Email does not contain the correct DE INNER CONTAINER COMMENT ", txt)
	}
	if !strings.Contains(txt, "https://test.anki.com/password-reset?user_id=USERIDBOB&token=TOKENBOB&dloc=") {
		t.Error("Email does not contain the correct URL for reset BOB ", txt)
	}
	if !strings.Contains(txt, "https://test.anki.com/password-reset?user_id=USERIDTOM&token=TOKENTOM&dloc=") {
		t.Error("Email does not contain the correct URL for reset TOM ", txt)
	}
	if !strings.Contains(txt, "Passwort für Anki-Konto zurücksetzen") {
		t.Error("Email does not contain the correct multi-user prompt ", txt)
	}
	if email.Headers["X-Anki-Tmpl"][0] != "ForgotPassword" {
		t.Fatal("Email does not have needed header ForgotPassword:", email.Headers)
	}
	if !strings.Contains(email.Subject, "Zurücksetzen des Passworts angefordert") {
		t.Error("Email subject is not in German: ", email.Subject)
	}
}

var locsTests = []struct {
	loc string
}{
	{"beta"},
	{"dev"},
	{""},
}

func TestDloc(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}

	for _, test := range locsTests {

		e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: test.loc}
		msg := VerificationEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", true, "1111-11-11", "TOKEN", util.Pstring("en"), "overdrive", testTime}, nil)
		email, err := sendEmail(msg, test.loc)

		if err != nil {
			t.Fatal("Error on send", err)
		}

		if email == nil {
			t.Fatal("No email created for Verification")
		}
		byt, _ := ioutil.ReadAll(email.TextBody)
		txt := string(byt)
		if !strings.Contains(txt, fmt.Sprintf("https://test.anki.com/email-verifying?user_id=USERID&token=TOKEN&dloc=%s", test.loc)) {
			t.Error("Email does not contain the correct URL for validation ", txt, test.loc)
		}
	}
}

func TestAnkiEmailer_AccountUpdatedEmaill(t *testing.T) {
	pmclient := &postmarkapp.Client{LastEmail: make(chan *postmarkapp.Message, 1)}
	e := &AnkiEmailer{TemplateDir: tmpldir, PostmarkC: pmclient, FormsUrlRoot: URL_ROOT, ApiLocator: "dev"}

	msg := AccountUpdatedEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", true, "1111-11-11", "TOKEN", util.Pstring("en"), "overdrive", testTime})
	email, err := sendEmail(msg, "dev")

	if err != nil {
		t.Fatal("Error on send", err)
	}

	if email == nil {
		t.Fatal("No email created for Account Updated")
	}
	byt, _ := ioutil.ReadAll(email.TextBody)
	txt := string(byt)
	if !strings.Contains(txt, "An Anki account associated with this email address has been updated.") {
		t.Error("Email does not contain the correct test for update notice: ", txt)
	}
}

var appCreatedTests = []struct {
	app  string
	lang string
}{
	{"overdrive", "en"},
	{"overdrive", "de"},
	{"drive", "en"},
	{"web", "en"},
	{"", "en"},
	{"misc", "de"},
	{"drive", "de"},
	{"web", "de"},
	{"", "de"},
	{"misc", "de"},
}

var appCreatedFuncs = []struct {
	fname string
	f     func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail
}{
	{"verification",
		func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail {
			return VerificationEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", adult, "1111-11-11", "TOKEN", util.Pstring(lang), app, testTime}, nil)
		},
	},
	{"Verification7DayReminderEmail",
		func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail {
			return Verification7DayReminderEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", adult, "1111-11-11", "TOKEN", util.Pstring(lang), app, testTime}, nil)
		},
	},
	{"Verification14DayReminderEmail",
		func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail {
			return Verification14DayReminderEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", adult, "1111-11-11", "TOKEN", util.Pstring(lang), app, testTime}, nil)
		},
	},
	{"ForgotPasswordEmail",
		func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail {
			return ForgotPasswordEmail(e, "Tom", "Eliaz", "tom@example.com",
				[]AccountInfo{
					{"USERNAMETOM", "USERIDTOM", adult, "0000-00-00", "TOKENTOM", util.Pstring(lang), app, testTime},
					{"USERNAMEBOB", "USERIDBOB", adult, "0000-00-00", "TOKENBOB", util.Pstring(lang), app, testTime}}, nil)
		},
	},
	{"PostVerificationEmail",
		func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail {
			return PostVerificationEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", adult, "1111-11-11", "TOKEN", util.Pstring(lang), app, testTime})
		},
	},
	{"AccountUpdatedEmail",
		func(e *AnkiEmailer, app, lang string, adult bool) *AnkiEmail {
			return AccountUpdatedEmail(e, "Tom", "Eliaz", "tom@example.com", AccountInfo{"USERNAME", "USERID", adult, "1111-11-11", "TOKEN", util.Pstring(lang), app, testTime})
		},
	},
}

func TestAppCreatedEmail(t *testing.T) {
	e := &AnkiEmailer{TemplateDir: tmpldir, FormsUrlRoot: URL_ROOT, ApiLocator: ""}
	for _, test := range appCreatedTests {
		for _, fun := range appCreatedFuncs {
			// Adult doesn't matter in terms of logo - but does let us run through all the different emails
			for _, adult := range []bool{true, false} {
				_, err := sendEmail(fun.f(e, test.app, test.lang, adult), "")
				if err != nil {
					t.Fatal("Error sending email", fun.fname, err)
				}
			}
		}
	}
}

type fakePostmarkBatchSend struct {
	callArgs []*postmarkapp.Message
	result   []*postmarkapp.Result
	err      error
}

func (fpm *fakePostmarkBatchSend) Send(msg *postmarkapp.Message) (*postmarkapp.Result, error) {
	panic("Not implemented")
}

func (fpm *fakePostmarkBatchSend) SendBatch(msgs []*postmarkapp.Message) ([]*postmarkapp.Result, error) {
	fpm.callArgs = msgs
	return fpm.result, fpm.err
}

var (
	stubFields = map[string]interface{}{
		"Firstname":        "first",
		"Lastname":         "last",
		"ToEmail":          "",
		"Username":         "",
		"CreatedByAppName": "",
	}
	bmsg1 = &AnkiEmail{
		TemplateName: PostVerificationUnder13Template,
		ToEmail:      "send1@example.net",
		Fields:       stubFields,
	}
	bmsg2 = &AnkiEmail{
		TemplateName: PostVerificationUnder13Template,
		ToEmail:      "nosend@example.com",
		Fields:       stubFields,
	}
	bmsg3 = &AnkiEmail{
		TemplateName: PostVerificationUnder13Template,
		ToEmail:      "send2@example.net",
		Fields:       stubFields,
	}
	bmsg4 = &AnkiEmail{
		TemplateName: PostVerificationUnder13Template,
		ToEmail:      "nosend2@example.com",
		Fields:       stubFields,
	}

	sendBatchTests = []struct {
		name      string
		msgs      []*AnkiEmail
		pmresult  []*postmarkapp.Result
		pmexpargs []string // email addresses
		expected  []string
	}{
		{
			name:      "skip1",
			msgs:      []*AnkiEmail{bmsg1, bmsg2, bmsg3},
			pmresult:  []*postmarkapp.Result{{Message: "bmsg1"}, {Message: "bmsg3"}},
			pmexpargs: []string{"send1@example.net", "send2@example.net"},
			expected:  []string{"send1@example.net", "nosend@example.com", "send2@example.net"},
		}, {
			name:      "all-example", // neither email should make it to postmark
			msgs:      []*AnkiEmail{bmsg2, bmsg4},
			pmresult:  []*postmarkapp.Result{{Message: "bmsg2"}, {Message: "bmsg4"}},
			pmexpargs: []string{},
			expected:  []string{"nosend@example.com", "nosend2@example.com"},
		},
	}
)

func setupTempDir() string {
	d, _ := ioutil.TempDir("", "emailtest")
	enDir := filepath.Join(d, "en")
	os.Mkdir(enDir, 0777)
	ioutil.WriteFile(filepath.Join(enDir, "plain.html"), []byte("html template"), 0666)
	ioutil.WriteFile(filepath.Join(enDir, "plain.txt"), []byte("text template"), 0666)
	return d
}

func TestSendBatchOK(t *testing.T) {
	pm := &fakePostmarkBatchSend{
		result: []*postmarkapp.Result{
			{Message: "bmsg1"}, {Message: "bmsg2"}, {Message: "bmsg3"}},
		err: nil,
	}
	emailer := &AnkiEmailer{
		PostmarkC: pm,
	}

	msgs := []*AnkiEmail{bmsg1, bmsg2, bmsg3}
	results, err := emailer.SendBatch(msgs)
	if err != nil {
		t.Fatal("SendBatch returned error", err)
	}

	emails := make([]string, 0)
	for _, r := range results {
		emails = append(emails, r.Email.ToEmail)
	}

	expected := []string{"send1@example.net", "nosend@example.com", "send2@example.net"}
	if !reflect.DeepEqual(emails, expected) {
		pretty.Println("Expected", expected)
		pretty.Println("Result  ", emails)
		t.Error("incorrect result")
	}

	callargs := make([]string, 0)
	for _, a := range pm.callArgs {
		callargs = append(callargs, a.To[0].Address)
	}
	if !reflect.DeepEqual(callargs, expected) {
		pretty.Println("Expected", expected)
		pretty.Println("Actual  ", callargs)
		t.Error("incorrect pm args")
	}
}

func TestSendBatchErr(t *testing.T) {
	// test hard error from postmark
	err := errors.New("Test Error")
	pm := &fakePostmarkBatchSend{
		err: err,
	}

	emailer := &AnkiEmailer{
		PostmarkC: pm,
	}
	results, err := emailer.SendBatch([]*AnkiEmail{bmsg1, bmsg2})
	if err == nil {
		t.Fatal("No error received")
	}
	if results != nil {
		t.Fatal("Unexpected results received", results)
	}
}

func TestSendBatchSingleErr(t *testing.T) {
	// Make sure that per-email errors from postmark are returned correctly
	err := errors.New("Test Error")
	pmResponse := &postmarkapp.Result{Message: "bmsg1", ErrorCode: 123}
	pm := &fakePostmarkBatchSend{
		result: []*postmarkapp.Result{pmResponse},
	}

	emailer := &AnkiEmailer{
		PostmarkC: pm,
	}
	results, err := emailer.SendBatch([]*AnkiEmail{bmsg1})
	if err != nil {
		t.Fatal("Unexpected error", err)
	}
	if len(results) != 1 {
		pretty.Println("RESULTS", results)
		t.Fatal("Incorrect result count")
	}
	if !reflect.DeepEqual(results[0].Error.result, pmResponse) {
		t.Fatalf("Result did not match resposne from PM actual=%#v expected=%#v", results[0].Error.result, pmResponse)
	}
}

func TestSendErrorSupportsJsonError(t *testing.T) {
	// This will fail to compile if SendError doesn't implement
	// jsonutil.JsonError
	var _ jsonutil.JsonError = (*SendError)(nil)
}
