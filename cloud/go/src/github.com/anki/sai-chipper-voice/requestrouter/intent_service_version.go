package requestrouter

import (
	"errors"
	"fmt"

	"github.com/anki/sai-go-util/log"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

type IntentServiceVersion struct {
	IntentSvc pb.IntentService

	// Dialogflow
	DFProjectName string
	DFVersion     string
	DFCredsKey    string

	// Lex
	LexBotName  string
	LexBotAlias string
}

var (
	ErrorVersionEmpty = errors.New("Version info empty, set either Dialogflow or Lex")
)

func (c *IntentServiceVersion) String() string {
	svc := "none"
	ver := ""
	name := ""
	if c.DFProjectName != "" {
		svc = c.IntentSvc.String()
		ver = c.DFVersion
		name = c.DFProjectName
	} else if c.LexBotName != "" {
		svc = c.IntentSvc.String()
		ver = c.LexBotName
		name = c.LexBotAlias
	}
	return fmt.Sprintf("svc: %s, ver: %s, proj: %s", svc, ver, name)
}

// Validate ensures that at least Dialogflow or Lex version is set
func (c *IntentServiceVersion) Validate() error {
	if (c.DFProjectName == "" || c.DFVersion == "") && (c.LexBotName == "" || c.LexBotAlias == "") {
		return ErrorVersionEmpty
	}
	alog.Info{"action": "validate_version", "status": alog.StatusOK, "version": c.String()}.Log()
	return nil
}

type IntentSvcVer func(c *IntentServiceVersion)

func NewIntentServiceVersion(cfgs ...IntentSvcVer) *IntentServiceVersion {
	c := &IntentServiceVersion{}
	for _, cfg := range cfgs {
		cfg(c)
	}
	return c
}

func WithIntentService(s *pb.IntentService) IntentSvcVer {
	return func(c *IntentServiceVersion) {
		if s != nil {
			c.IntentSvc = *s
		}
	}

}

func WithProjectName(name *string) IntentSvcVer {
	return func(c *IntentServiceVersion) {
		if name != nil && *name != "" {
			c.DFProjectName = *name
		}
	}
}

func WithDFVersion(ver *string) IntentSvcVer {
	return func(c *IntentServiceVersion) {
		if ver != nil && *ver != "" {
			c.DFVersion = *ver
		}
	}
}

func WithDFCredsKey(keyName *string) IntentSvcVer {
	return func(c *IntentServiceVersion) {
		if keyName != nil && *keyName != "" {
			c.DFCredsKey = *keyName
		}
	}
}

func WithLexBot(botName *string, botAlias *string) IntentSvcVer {
	return func(c *IntentServiceVersion) {
		if botName != nil && *botName != "" && botAlias != nil && *botAlias != "" {
			c.LexBotName = *botName
			c.LexBotAlias = *botAlias
		}
	}
}
