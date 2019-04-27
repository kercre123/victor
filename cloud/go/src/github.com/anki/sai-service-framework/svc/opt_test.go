package svc

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func Test_ParamToEnvVarHelper(t *testing.T) {
	assert.Equal(t, "ENV_VAR", toEnv("env-var"))
	assert.Equal(t, "ENV_VAR", toEnv("ENV-var"))
	assert.Equal(t, "ENV_VAR", toEnv("env-Var"))
	assert.Equal(t, "ENV_VAR", toEnv("envVar"))
}

func Test_DurationHelper(t *testing.T) {
	assert.Equal(t, time.Minute*5, Duration(time.Minute*5).Duration())
	var d Duration
	assert.NoError(t, d.Set("5m"))
	assert.Equal(t, time.Minute*5, d.Duration())
	assert.Error(t, d.Set("whatever"))
}
