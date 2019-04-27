package svc

import (
	"os"
	"testing"

	"github.com/jawher/mow.cli"
	"github.com/stretchr/testify/assert"
)

func TestLogComponent_DefaultLogSource(t *testing.T) {
	assert.Equal(t, "sai_token_service", toLogSource("sai-token-service"))
	assert.Equal(t, "sai_token_service", toLogSource("/sai-token-service"))
	assert.Equal(t, "sai_token_service", toLogSource("./sai-token-service"))
	assert.Equal(t, "sai_token_service", toLogSource("SAI-TOKEN-SERVICE"))
	assert.Equal(t, "sai_token_service", toLogSource("/SAI-TOKEN-SERVICE"))
	assert.Equal(t, "sai_token_service", toLogSource("./SAI-TOKEN-SERVICE"))
	assert.Equal(t, "sai_token_service", toLogSource("sai_token_service"))
	assert.Equal(t, "sai_token_service", toLogSource("/sai_token_service"))
	assert.Equal(t, "sai_token_service", toLogSource("./sai_token_service"))
}

func TestLogComponent_CommandOptions(t *testing.T) {
	app := cli.App("test", "test")
	c := &LogComponent{}

	app.Spec = c.CommandSpec()
	c.CommandInitialize(app.Cmd)
	app.Action = func() {}
	app.Run([]string{
		"test",
		"--log-type", "stdout",
		"--log-level", "DEBUG",
		"--log-source", "test_source",
		"--log-sourcetype", "test_sourcetype",
		"--log-index", "test_index",
		"--log-kinesis-stream", "test_stream",
	})

	assert.Equal(t, "stdout", *c.logType)
	assert.Equal(t, "DEBUG", *c.level)
	assert.Equal(t, "test_source", *c.source)
	assert.Equal(t, "test_sourcetype", *c.sourceType)
	assert.Equal(t, "test_index", *c.index)
	assert.Equal(t, "test_stream", *c.kinesisStream)
}

func TestLogComponent_EnvVars(t *testing.T) {
	os.Setenv("LOG_TYPE", "kinesis")
	os.Setenv("LOG_LEVEL", "WARN")
	os.Setenv("LOG_SOURCE", "TestLogComponent_EnvVars")
	os.Setenv("LOG_SOURCETYPE", "SourceTypeFromVar")
	os.Setenv("LOG_INDEX", "IndexFromVar")
	os.Setenv("LOG_KINESIS_STREAM", "Oh what a wonderful world")

	app := cli.App("test", "test")
	c := &LogComponent{}

	app.Spec = c.CommandSpec()
	c.CommandInitialize(app.Cmd)
	app.Action = func() {}

	app.Run([]string{
		"test",
	})

	assert.Equal(t, "kinesis", *c.logType)
	assert.Equal(t, "WARN", *c.level)
	assert.Equal(t, "TestLogComponent_EnvVars", *c.source)
	assert.Equal(t, "SourceTypeFromVar", *c.sourceType)
	assert.Equal(t, "IndexFromVar", *c.index)
	assert.Equal(t, "Oh what a wonderful world", *c.kinesisStream)
}
