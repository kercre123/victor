package testclient

import (
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-go-util/log"
)

func (c *ChipperClient) TextIntent(text string) *pb.IntentResult {
	r := &pb.TextRequest{
		TextInput:       text,
		DeviceId:        c.Opts.DeviceId,
		Session:         c.Opts.Session,
		LanguageCode:    c.Opts.LangCode,
		IntentService:   c.Opts.Service,
		Mode:            c.Opts.Mode,
		FirmwareVersion: c.Opts.Fw,
		SkipDas:         c.Opts.SkipDas,
	}

	res, err := c.Client.TextIntent(c.Ctx, r)
	if err != nil {
		alog.Error{"action": "get_text_intent", "status": "fail", "error": err}.Log()
		return nil
	} else {
		alog.Info{"action": "get_text_intent", "status": "success",
			"intent": res.IntentResult.Action,
			"text":   res.IntentResult.QueryText}.Log()
	}
	return res.IntentResult
}
