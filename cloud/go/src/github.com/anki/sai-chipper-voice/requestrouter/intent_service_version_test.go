package requestrouter

import (
	"github.com/stretchr/testify/assert"
	"testing"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

type testVersion struct {
	intentSvc  pb.IntentService
	dfProj     string
	dfVersion  string
	lexName    string
	lexVersion string
	err        error
}

func TestVersionValidation(t *testing.T) {
	testCases := []testVersion{
		{df, "victor-dev-en-us", "df-dev-0_12", "", "", nil},
		{lex, "", "", "victor-dev", "lex_dev_a_bc", nil},
		{df, "victor-dev-en-us-bad", "", "", "", ErrorVersionEmpty},
		{df, "", "df-dev-0_12-bad-1", "", "", ErrorVersionEmpty},
		{df, "", "df-dev-0_12-bad-2", "victor-dev", "", ErrorVersionEmpty},
		{lex, "victor-dev-en-us-bad-3", "", "", "lex_dev_a_cd", ErrorVersionEmpty},
	}

	for _, tc := range testCases {
		version := NewIntentServiceVersion(
			WithIntentService(&tc.intentSvc),
			WithProjectName(&tc.dfProj),
			WithDFVersion(&tc.dfVersion),
			WithLexBot(&tc.lexName, &tc.lexVersion),
		)
		err := version.Validate()
		assert.Equal(t, tc.err, err)
		if err == nil {
			if tc.intentSvc == df {
				assert.Equal(t, tc.dfVersion, version.DFVersion)
				assert.Equal(t, tc.dfProj, version.DFProjectName)
			} else {
				assert.Equal(t, tc.lexVersion, version.LexBotAlias)
				assert.Equal(t, tc.lexName, version.LexBotName)
			}
		}
	}
}
