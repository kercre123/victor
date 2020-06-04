package tokenmetrics

import (
	"github.com/anki/sai-go-util/metricsutil"
)

var (
	Registry = metricsutil.NewRegistry("service.sai_token_service")
)
