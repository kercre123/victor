package requestrouter

import (
	"errors"
	"fmt"
	"time"

	"github.com/anki/sai-go-util/log"
)

const (
	maxRatioSum = 100
)

type RatioValue struct {
	Dialogflow int `json:"df"`
	Lex        int `json:"lex"`
	MS         int `json:"ms"`
}
type Ratio struct {
	Date      string
	Value     RatioValue
	CreatedBy string
}

var (
	ErrorRatioNot100 = errors.New("Sum of ratio must be equal to 100")
	DefaultRatio     = Ratio{
		Value: RatioValue{
			Dialogflow: 100,
			Lex:        0,
			MS:         0,
		},
		CreatedBy: "default",
	}
)

func (r *Ratio) String() string {
	return fmt.Sprintf("ratio: {df: %d, lex: %d, ms:%d}, date: %s, creator: %s",
		r.Value.Dialogflow, r.Value.Lex, r.Value.MS,
		r.Date, r.CreatedBy,
	)
}

func (r *Ratio) Check() error {
	sum := r.Value.Dialogflow + r.Value.Lex + r.Value.MS
	if sum != maxRatioSum {
		alog.Warn{
			"action": "check_ratio",
			"status": alog.StatusError,
			"ratio":  r,
			"sum":    sum,
		}.Log()
		return ErrorRatioNot100
	}
	return nil
}

func NewRatio(df, lex, ms int, creator string) (*Ratio, error) {
	r := &Ratio{
		Date: GetTimeStamp(),
		Value: RatioValue{
			Dialogflow: df,
			Lex:        lex,
			MS:         ms,
		},
		CreatedBy: creator,
	}

	if err := r.Check(); err != nil {
		return nil, err
	}
	alog.Debug{"action": "new_ratio", "ratio": r.String()}.Log()
	return r, nil
}

func GetTimeStamp() string {
	return time.Now().Format("2006-01-02 15:04:05.9999")
}
