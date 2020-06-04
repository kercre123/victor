// Emails is an accounts package that handles sending of all email communication to the users.
// We explicitly use text/template not html/template as we try to reuse code for txt and html templates
// and the html/template package was having trouble not encoding .URLs in templates without <a></a> context.
package email

import (
	"bytes"
	"fmt"
	htmltemplate "html/template"
	"io"
	"net/http"
	"net/mail"
	"path"
	"strings"
	texttemplate "text/template"
	"time"

	"github.com/GeertJohan/go.rice"
	"github.com/anki/sai-go-util/envconfig"
	"github.com/anki/sai-go-util/jsonutil"
	"github.com/anki/sai-go-util/log"
	"github.com/anki/sai-go-util/postmarkapp"
)

const (
	Root = "email_root"

	VerificationOver13Template         = "email_verification"
	VerificationUnder13Template        = "email_verification_under13"
	PostVerificationUnder13Template    = "email_parental_post_verification"
	VerifyReminder7DayOver13Template   = "email_verification_7day_reminder"
	VerifyReminder7DayUnder13Template  = "email_verification_under13_7day_reminder"
	VerifyReminder14DayOver13Template  = "email_verification_14day_reminder"
	VerifyReminder14DayUnder13Template = "email_verification_under13_14day_reminder"
	ForgotTemplateName                 = "email_forgotpassword"
	AccountUpdatedEmailTemplate        = "email_verification_account_updated"
)

var (
	KnownLang = map[string]bool{
		"en": true,
		"de": true,
		"fr": true,
	}
	SubjectMap = map[string]map[string]string{
		VerificationOver13Template:         {"en": "Activate Your Anki Account!", "de": "Anki-Konto aktivieren!", "fr": "Activer votre compte Anki."},
		VerificationUnder13Template:        {"en": "Request to Activate Your Child’s Anki Account", "de": "Anki-Konto Ihres Kindes aktivieren", "fr": "Activer le compte Anki de votre enfant."},
		PostVerificationUnder13Template:    {"en": "Anki Account Activated", "de": "Anki-Konto – Aktivierung erfolgreich", "fr": "Compte Anki activé"},
		VerifyReminder7DayOver13Template:   {"en": "One Week Left to Activate Your Anki Account!", "de": "Anki-Konto aktivieren: noch 1 Woche", "fr": "Plus qu'une semaine pour activer votre compte Anki !"},
		VerifyReminder7DayUnder13Template:  {"en": "You Have One Week Left to Activate Your Child’s Anki Account", "de": "Anki-Konto Ihres Kindes aktivieren – noch 1 Woche", "fr": "Plus qu'une semaine pour activer le compte Anki de votre enfant !"},
		VerifyReminder14DayOver13Template:  {"en": "Last Day to Activate Your Anki Account!", "de": "Anki-Konto aktivieren – Deine letzte Chance!", "fr": "Plus qu'un jour pour activer votre compte Anki !"},
		VerifyReminder14DayUnder13Template: {"en": "One Day Left to Activate Your Child’s Anki Account", "de": "Anki-Konto aktivieren: noch 1 Tag", "fr": "Plus qu'un jour pour activer le compte Anki de votre enfant !"},
		ForgotTemplateName:                 {"en": "Anki Account Password Reset Requested", "de": "Zurücksetzen des Passworts angefordert", "fr": "Demande de réinitialisation de mot de passe."},
		AccountUpdatedEmailTemplate:        {"en": "Your Anki Account Details Have Been Updated", "de": "Aktualisierung Deiner Anki-Kontodaten", "fr": "Votre compte Anki a été mis à jour."},
	}

	Emailer                *AnkiEmailer
	PostmarkApiKey         string
	EmailTemplateDirectory string
	FormsUrlRoot           string
	ApiLocator             string
)

func init() {
	envconfig.String(&PostmarkApiKey, "POSTMARK_API_KEY", "", "Postmark API key (POSTMARK_API_TEST for testing.")
}

func InitAnkiEmailer() {
	pmclient := &(postmarkapp.Client{ApiKey: PostmarkApiKey})
	Emailer = &AnkiEmailer{TemplateDir: EmailTemplateDirectory, PostmarkC: pmclient, FormsUrlRoot: FormsUrlRoot, ApiLocator: ApiLocator}

	if PostmarkApiKey == "" {
		alog.Error{"action": "email_init", "status": "panic_no_pmapik", "error": "Did you want POSTMARK_API_KEY=NO_EMAIL ?"}.Log()
		panic("Email module: POSTMARK_API_KEY Not Set")
	}

	if PostmarkApiKey != "NO_EMAIL" && FormsUrlRoot == "" {
		alog.Error{"action": "email_init", "status": "panic_bad_urlroot", "error": "URL_ROOT Not Set"}.Log()
		panic("Email Module: URL_ROOT not set")
	}
}

// This cannot be run from init() as the rice box may not yet be available.
func VerifyEmailer() {
	// If we ever want to instatiate other AnkiEmailers with different template directories
	// We'll need to use an initialization function for object creation that runs these checks.
	// For now we can do it at the module level.
	box, err := rice.FindBox("templates")
	if err != nil {
		alog.Error{"action": "email_init", "status": "panic_bad_tmpl", "error": "Failed to open template box", "box_error": err}.Log()
		panic("Email module: Failed to load template box")
	}

	if _, err := templateFromBoxFiles(box, false, "en/"+Root+".txt", "en/"+VerificationOver13Template+".txt"); err != nil {
		alog.Error{"action": "email_init", "status": "panic_bad_tmpl", "error": "Failed to parse template", "parse_error": err}.Log()
		panic("Email Module failed to parse templates")
	}
}

type PostmarkSender interface {
	Send(msg *postmarkapp.Message) (*postmarkapp.Result, error)
	SendBatch(msg []*postmarkapp.Message) ([]*postmarkapp.Result, error)
}

var (
	ServerError       = -1
	InvalidEmail      = 300
	InactiveRecipient = 406
)

type SendError struct {
	err    error
	result *postmarkapp.Result
}

func (se SendError) Code() int {
	if se.result == nil {
		return ServerError
	}
	return se.result.ErrorCode
}

func (se SendError) Error() string {
	if se.result == nil {
		return se.err.Error()
	}
	return se.result.Message
}

func (se SendError) JsonError() jsonutil.JsonErrorResponse {
	if se.result == nil {
		return jsonutil.JsonErrorResponse{
			HttpStatus: http.StatusInternalServerError,
			Message:    se.Error(),
		}
	}
	return jsonutil.JsonErrorResponse{
		HttpStatus: http.StatusBadRequest,
		Code:       "email_send_error",
		Message:    se.result.Message,
		Metadata:   map[string]interface{}{"email_send_error_code": se.result.ErrorCode},
	}
}

// AnkiEmailer holds the interface to sending Anki's specific emails using the postmarkapp API
// This is used internally
type AnkiEmailer struct {
	TemplateDir  string // Root template directory
	PostmarkC    PostmarkSender
	FormsUrlRoot string
	ApiLocator   string
}

// Send takes a single AnkiEmail message and dispatches it via Postmark.
// Emails to example.com are logged here, and sending is blocked at the postmarkapp API
func (s *AnkiEmailer) Send(e *AnkiEmail) error {
	msg, err := e.Message()
	if err != nil {
		return err
	}

	if strings.HasSuffix(strings.ToLower(e.ToEmail), "@example.com") {
		alog.Info{"action": "email", "status": "example.com", "TemplateName": e.TemplateName, "to": e.ToEmail}.Log()
	}

	var res *postmarkapp.Result
	alog.Info{"action": "postmark_send"}.TimeIt(func() error {
		res, err = s.PostmarkC.Send(msg)
		return err
	})
	if err != nil || res.ErrorCode != 0 {
		return &SendError{err: err, result: res}
	}
	return nil
}

type SendBatchResult struct {
	Email *AnkiEmail
	Error *SendError
}

// SendBatch sends multiple to Postmark using a single API call.
// Emails to addresses @example.com are filtered out at the postmarkapp level
func (s *AnkiEmailer) SendBatch(emails []*AnkiEmail) (results []*SendBatchResult, err error) {
	var msgs []*postmarkapp.Message
	for _, e := range emails {
		msg, err := e.Message()
		if err != nil {
			return nil, err
		}
		if strings.HasSuffix(strings.ToLower(e.ToEmail), "@example.com") {
			alog.Info{"action": "email", "status": "example.com", "TemplateName": e.TemplateName, "to": e.ToEmail}.Log()
		}
		msgs = append(msgs, msg)
	}

	var pmr []*postmarkapp.Result
	alog.Info{"action": "postmark_batch_send", "msg_count": len(msgs)}.TimeIt(func() error {
		pmr, err = s.PostmarkC.SendBatch(msgs)
		return err
	})

	if err != nil {
		return nil, err
	}

	for i, res := range pmr {
		result := &SendBatchResult{
			Email: emails[i],
		}
		if res.ErrorCode != 0 {
			result.Error = &SendError{result: res}
		}
		results = append(results, result)
	}

	return results, nil
}

// AnkiEmail describes one email to be sent ay an AnkiEmailer.
type AnkiEmail struct {
	// TemplateName should not have a .html/.txt extension
	TemplateName string
	// EmailName is referenced by unit tests
	EmailName string
	EmailLang *string
	ToName    string
	ToEmail   string
	Tag       string
	// Fields holds the fields to pass to the templates
	// Fields should not be escpae for html; the template system will take care of that.
	Fields   map[string]interface{}
	Metadata interface{} // Any additional context data the caller may need returned in a result resposne
}

func getSubject(templateName, lang string) (string, bool) {
	subjectOptions, ok := SubjectMap[templateName]
	if !ok {
		return "", false
	}
	subject, ok := subjectOptions[lang]
	if !ok {
		return "", false
	}
	return subject, true
}

// Message builds and renders the text and html templates for an email
// and compiles it into a postmarkapp.Message ready to be sent.
func (e *AnkiEmail) Message() (*postmarkapp.Message, error) {
	var (
		err               error
		htmlTmpl, txtTmpl Template
		htmlBody, txtBody bytes.Buffer
		lang              string
	)
	box, err := rice.FindBox("templates")
	if err != nil {
		return nil, fmt.Errorf("Failed to load templates box: %s", err)
	}

	lang = emailLangToTemplatePath(e.EmailLang)

	if htmlTmpl, err = templateFromBoxFiles(box, true, path.Join(lang, Root+".html"), path.Join(lang, e.TemplateName+".html")); err != nil {
		fmt.Println("PATH", path.Join(lang, e.TemplateName+".html"))
		return nil, fmt.Errorf("Failed to load HTML template for %s: %s %s", lang, e.TemplateName, err)
	}
	if txtTmpl, err = templateFromBoxFiles(box, false, path.Join(lang, Root+".txt"), path.Join(lang, e.TemplateName+".txt")); err != nil {
		return nil, fmt.Errorf("Failed to load text template for %s: %s %s", lang, e.TemplateName, err)
	}

	if err = htmlTmpl.Execute(&htmlBody, e.Fields); err != nil {
		return nil, fmt.Errorf("Failed to execute HTML template for %s: %s %s", lang, e.TemplateName, err)
	}
	if err = txtTmpl.Execute(&txtBody, e.Fields); err != nil {
		return nil, fmt.Errorf("Failed to execute text template for %s: %s %s", lang, e.TemplateName, err)
	}

	subject, ok := getSubject(e.TemplateName, lang)
	if !ok {
		return nil, fmt.Errorf("Failed to find subject for lang: template %s: %s", lang, e.TemplateName)
	}

	m := &postmarkapp.Message{
		From: &mail.Address{
			Name:    "Anki Accounts",
			Address: "accountmail@anki.com",
		},
		To: []*mail.Address{
			{Name: e.ToName, Address: e.ToEmail},
		},
		ReplyTo: &mail.Address{
			Name:    "Anki Support",
			Address: "support@anki.com",
		},
		Subject:  subject,
		TextBody: &txtBody,
		HtmlBody: &htmlBody,
		Tag:      e.Tag,
		Headers:  mail.Header{"X-Anki-Tmpl": []string{e.EmailName}},
	}

	return m, nil
}

// AccountInfo is used as an input to the email methods, and contains information needed to create the links.
type AccountInfo struct {
	Username         string
	UserId           string
	Adult            bool
	DOB              string
	Token            string
	EmailLang        *string
	CreatedByAppName string
	TimeCreated      time.Time
}

// Template provides an abstract interface for the text and html
// templates we use with emails.
type Template interface {
	Execute(wr io.Writer, data interface{}) (err error)
	New(name string) Template
	Parse(text string) (Template, error)
}

type htmlTemplate struct {
	*htmltemplate.Template
}

func (t *htmlTemplate) New(name string) Template {
	return &htmlTemplate{t.Template.New(name)}
}

func (t *htmlTemplate) Parse(text string) (Template, error) {
	r, err := t.Template.Parse(text)
	if err != nil {
		return &htmlTemplate{r}, nil
	}
	return nil, err
}

type textTemplate struct {
	*texttemplate.Template
}

func (t *textTemplate) New(name string) Template {
	return &textTemplate{t.Template.New(name)}
}

func (t *textTemplate) Parse(text string) (Template, error) {
	r, err := t.Template.Parse(text)
	if err != nil {
		return &textTemplate{r}, nil
	}
	return nil, err
}

var templateCache = make(map[string]Template)

// Load templates from disk or from an embeded string generated by the rice command.
// Analogous to template.ParseFiles
func templateFromBoxFiles(box *rice.Box, isHtml bool, filenames ...string) (result Template, err error) {
	cacheKey := strings.Join(filenames, ";")
	if t, ok := templateCache[cacheKey]; ok {
		return t, nil
	}

	for _, filename := range filenames {
		var tmpl Template
		content, err := box.String(filename)
		if err != nil {
			return nil, err
		}
		if result == nil {
			if isHtml {
				tmpl = &htmlTemplate{htmltemplate.New(filename)}
			} else {
				tmpl = &textTemplate{texttemplate.New(filename)}
			}
			result = tmpl
		} else {
			tmpl = result.New(filename)
		}
		if _, err = tmpl.Parse(content); err != nil {
			return nil, err
		}
	}
	templateCache[cacheKey] = result
	return result, nil
}

// Map EmailLang
func emailLangToTemplatePath(emailLang *string) (templateLang string) {
	if emailLang != nil && *emailLang != "" && len(*emailLang) >= 2 {
		maplang := (*emailLang)[0:2]
		if _, ok := KnownLang[maplang]; ok {
			return maplang
		}
	}
	return "en" // Default map language is English
}

type Email struct {
	TemplateName string // without .html/.txt extension
	ToName       string
	ToEmail      string
	Fields       map[string]interface{}
}

// Method signature for Verification* email functions
type VerificationEmailFunc func(s *AnkiEmailer, firstName, lastName, toEmail string, userInfo AccountInfo, r *http.Request) *AnkiEmail

func VerificationEmail(s *AnkiEmailer, firstName, lastName, toEmail string, userInfo AccountInfo, r *http.Request) *AnkiEmail {
	var templateName, emailName string

	if userInfo.Adult {
		templateName = VerificationOver13Template
		emailName = "Verification"
	} else {
		templateName = VerificationUnder13Template
		emailName = "VerificationUnder13"
	}

	return &AnkiEmail{
		TemplateName: templateName,
		EmailName:    emailName,
		EmailLang:    userInfo.EmailLang,
		ToName:       firstName,
		ToEmail:      toEmail,
		Tag:          "account_verify", // XXX constant
		Fields: map[string]interface{}{
			"Firstname":        firstName,
			"Lastname":         lastName,
			"ToEmail":          toEmail,
			"Username":         userInfo.Username,
			"Dob":              userInfo.DOB,
			"CreatedByAppName": userInfo.CreatedByAppName,
			"ASSETURL":         fmt.Sprintf("%s/email/email_verify", s.FormsUrlRoot),
			"URL":              fmt.Sprintf("%s/email-verifying?user_id=%s&token=%s&dloc=%s", s.FormsUrlRoot, userInfo.UserId, userInfo.Token, s.ApiLocator),
		},
	}
}

func Verification7DayReminderEmail(s *AnkiEmailer, firstName, lastName, toEmail string, userInfo AccountInfo, r *http.Request) *AnkiEmail {
	var templateName, emailName string

	if userInfo.Adult {
		templateName = VerifyReminder7DayOver13Template
		emailName = "Verification7dReminder"
	} else {
		templateName = VerifyReminder7DayUnder13Template
		emailName = "Verification7dReminderUnder13"
	}

	return &AnkiEmail{
		TemplateName: templateName,
		EmailName:    emailName,
		EmailLang:    userInfo.EmailLang,
		ToName:       firstName,
		ToEmail:      toEmail,
		Tag:          "account_verify_reminder", // XXX constant
		Fields: map[string]interface{}{
			"Firstname":        firstName,
			"Lastname":         lastName,
			"ToEmail":          toEmail,
			"Username":         userInfo.Username,
			"Dob":              userInfo.DOB,
			"CreatedTimeEN":    userInfo.TimeCreated.Format("2 Jan 2006"),
			"CreatedTimeDE":    userInfo.TimeCreated.Format("02.01.2006"),
			"CreatedByAppName": userInfo.CreatedByAppName,
			"ASSETURL":         fmt.Sprintf("%s/email/email_verify", s.FormsUrlRoot),
			"URL":              fmt.Sprintf("%s/email-verifying?user_id=%s&token=%s&dloc=%s", s.FormsUrlRoot, userInfo.UserId, userInfo.Token, s.ApiLocator),
		},
	}
}

func Verification14DayReminderEmail(s *AnkiEmailer, firstName, lastName, toEmail string, userInfo AccountInfo, r *http.Request) *AnkiEmail {
	var templateName, emailName string

	if userInfo.Adult {
		templateName = VerifyReminder14DayOver13Template
		emailName = "Verification14dReminder"
	} else {
		templateName = VerifyReminder14DayUnder13Template
		emailName = "Verification14dReminderUnder13"
	}

	return &AnkiEmail{
		TemplateName: templateName,
		EmailName:    emailName,
		EmailLang:    userInfo.EmailLang,
		ToName:       firstName,
		ToEmail:      toEmail,
		Tag:          "account_verify_reminder",
		Fields: map[string]interface{}{
			"Firstname":        firstName,
			"Lastname":         lastName,
			"ToEmail":          toEmail,
			"Username":         userInfo.Username,
			"Dob":              userInfo.DOB,
			"CreatedTimeEN":    userInfo.TimeCreated.Format("2 Jan 2006"),
			"CreatedTimeDE":    userInfo.TimeCreated.Format("02.01.2006"),
			"CreatedByAppName": userInfo.CreatedByAppName,
			"ASSETURL":         fmt.Sprintf("%s/email/email_verify", s.FormsUrlRoot),
			"URL":              fmt.Sprintf("%s/email-verifying?user_id=%s&token=%s&dloc=%s", s.FormsUrlRoot, userInfo.UserId, userInfo.Token, s.ApiLocator),
		},
	}
}

func ForgotPasswordEmail(s *AnkiEmailer, firstName, lastName, toEmail string, accinfos []AccountInfo, r *http.Request) *AnkiEmail {
	accUrls := make(map[string]string)
	var emailLang *string
	var createdByAppName string

	for _, ai := range accinfos {
		accUrls[ai.Username] = fmt.Sprintf("%s/password-reset?user_id=%s&token=%s&dloc=%s", s.FormsUrlRoot, ai.UserId, ai.Token, s.ApiLocator)
		emailLang = ai.EmailLang // pick the last one
		createdByAppName = ai.CreatedByAppName
	}
	return &AnkiEmail{
		TemplateName: ForgotTemplateName,
		EmailName:    "ForgotPassword", // TODO: constant
		EmailLang:    emailLang,
		ToName:       firstName,
		ToEmail:      toEmail,
		Tag:          "account_forgotpassword", // XXX constant
		Fields: map[string]interface{}{
			"Firstname":        firstName,
			"Accounts":         accUrls,
			"ToEmail":          toEmail,
			"CreatedByAppName": createdByAppName,
			"ASSETURL":         fmt.Sprintf("%s/email/email_pass-reset", s.FormsUrlRoot),
		},
	}
}

func PostVerificationEmail(s *AnkiEmailer, firstName, lastName, toEmail string, userInfo AccountInfo) *AnkiEmail {
	return &AnkiEmail{
		TemplateName: PostVerificationUnder13Template,
		EmailName:    "PostVerificationUnder13",
		EmailLang:    userInfo.EmailLang,
		ToName:       firstName,
		ToEmail:      toEmail,
		Tag:          "account_postverification",
		Fields: map[string]interface{}{
			"Firstname":        firstName,
			"Lastname":         lastName,
			"ToEmail":          toEmail,
			"Username":         userInfo.Username,
			"CreatedByAppName": userInfo.CreatedByAppName,
		},
	}
}

func AccountUpdatedEmail(s *AnkiEmailer, firstName, lastName, toEmail string, userInfo AccountInfo) *AnkiEmail {
	return &AnkiEmail{
		TemplateName: AccountUpdatedEmailTemplate,
		EmailName:    "accountUpdatedEmail",
		EmailLang:    userInfo.EmailLang,
		ToName:       firstName,
		ToEmail:      toEmail,
		Tag:          "account_updated",
		Fields: map[string]interface{}{
			"Firstname":        firstName,
			"Lastname":         lastName,
			"ToEmail":          toEmail,
			"Username":         userInfo.Username,
			"CreatedByAppName": userInfo.CreatedByAppName,
		},
	}
}
