package requestrouter

import (
	"github.com/stretchr/testify/assert"
	"testing"

	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

type testSvcHash struct {
	svc       string
	lang      string
	mode      string
	hashKey   string
	intentSvc *pb.IntentService
	err       error
}

var (
	df  = pb.IntentService_DIALOGFLOW
	lex = pb.IntentService_LEX
)

func TestGetServiceHashKey(t *testing.T) {
	testCases := []testSvcHash{
		{"dialogflow", "en", "vc", "dialogflow_en-us_vc", &df, nil},
		{"df", "en", "vc", "dialogflow_en-us_vc", &df, nil},
		{"dialogflow", "en-us", "vc", "dialogflow_en-us_vc", &df, nil},
		{"dialogflow", "en-us", "vc", "dialogflow_en-us_vc", &df, nil},
		{"dialogflow", "fr", "vc", "dialogflow_fr-fr_vc", &df, nil},
		{"dialogflow", "fr-fr", "vc", "dialogflow_fr-fr_vc", &df, nil},
		{"dialogflow", "de", "vc", "dialogflow_de_vc", &df, nil},
		{"lex", "en", "vc", "lex_en-us_vc", &lex, nil},
		{"lex", "en-us", "vc", "lex_en-us_vc", &lex, nil},
		{"lex", "fr", "vc", "", nil, ErrorInvalidLexHash},
		{"blah", "en", "vc", "", nil, ErrorInvalidServiceHash},
	}

	for _, tc := range testCases {
		svcHash, err := GetServiceHashKey(tc.svc, tc.lang, tc.mode)
		assert.Equal(t, tc.err, err)
		if err == nil {
			assert.Equal(t, tc.hashKey, svcHash.Key)
		}
	}
}

func TestGetChipperIntentServiceOK(t *testing.T) {
	testCases := []testSvcHash{
		{hashKey: "dialogflow_en-us_vc", intentSvc: &df},
		{hashKey: "dialogflow_en-gb_game", intentSvc: &df},
		{hashKey: "dialogflow_de_vc", intentSvc: &df},
		{hashKey: "dialogflow_fr-fr_vc", intentSvc: &df},
		{hashKey: "dialogflow_en-us_vc", intentSvc: &df},
		{hashKey: "dialogflow_en-gb_vc", intentSvc: &df},
		{hashKey: "dialogflow_de_vc", intentSvc: &df},
		{hashKey: "dialogflow_fr-fr_vc", intentSvc: &df},
		{hashKey: "lex_en-us_vc", intentSvc: &lex},
		{hashKey: "lex_en-us_vc", intentSvc: &lex},
	}
	for _, tc := range testCases {
		res := DeconstructServiceHash(tc.hashKey)
		if res != nil {
			assert.Equal(t, *tc.intentSvc, *res.PbService)
		}
	}
}

func TestGetChipperIntentServiceError(t *testing.T) {
	testCases := []testSvcHash{
		{hashKey: "blah_en-us_vc", intentSvc: nil},
		{hashKey: "blah-blah-vc", intentSvc: nil},
	}
	for _, tc := range testCases {
		res := DeconstructServiceHash(tc.hashKey)
		assert.Nil(t, res)
	}
}

func TestNormalizeFwVersionNoErrors(t *testing.T) {
	testCases := []struct {
		fw   string
		norm string
		err  error
	}{
		{"0.12", "00.12.00", nil},
		{"0.01", "00.01.00", nil},
		{"0.1", "00.01.00", nil},
		{"0.2", "00.02.00", nil},
		{"0.10", "00.10.00", nil},
		{"1.2", "01.02.00", nil},
		{"1.25", "01.25.00", nil},
		{"11.5", "11.05.00", nil},
		{"99.5", "99.05.00", nil},
		{"0.14.0", "00.14.00", nil},
		{"1.0.0", "01.00.00", nil},
		{"1.2.0", "01.02.00", nil},
		{"1.2.1", "01.02.01", nil},
		{"1.2.25", "01.02.25", nil},
		{"1.10.0", "01.10.00", nil},
		{"1.10.7", "01.10.07", nil},
		{"1.10.37", "01.10.37", nil},
		{"17.0.0", "17.00.00", nil},
		{"17.0.1", "17.00.01", nil},
		{"17.0.13", "17.00.13", nil},
		{"17.6.1", "17.06.01", nil},
		{"17.23.13", "17.23.13", nil},
	}

	for _, tc := range testCases {
		n, err := NormalizeFwVersion(tc.fw)
		assert.NoError(t, err)
		assert.Equal(t, tc.norm, n)
	}
}

func TestNormalizeFwVersionWithErrors(t *testing.T) {
	testCases := []string{
		"123", "1.1.1.1", "whatever",
		"1.101", "101.0",
		"0.0.1000", "0.200.1", "103.3.1",
	}
	for _, tc := range testCases {
		_, err := NormalizeFwVersion(tc)
		assert.Error(t, err)
	}
}

func TestExtractVersionString(t *testing.T) {
	testCases := []struct {
		fw     string
		ver    string
		dasVer string
	}{
		{"", DefaultFWString, DefaultDasRobotVersionString},
		{"testing", DefaultFWString, DefaultDasRobotVersionString},
		{"v0.13.1548-563124f_os0.13.1548d-cd265eb-201808191433", "00.13.00", "0.13.0.1548"},
		{"v0.13.1561-0c69d2b_os0.13.1561d-a7d5368-201808212033", "00.13.00", "0.13.0.1561"},
		{"v0.13.1571-af5e82f_os0.13.1571d-8252218-201808222034", "00.13.00", "0.13.0.1571"},
		{"v0.13.1552-7435f92_os0.13.1552d-ce261ea-201808201634", "00.13.00", "0.13.0.1552"},
		{"v0.14.1601-91a7189_os0.13.1601d-7f6ef04-201808281534", "00.14.00", "0.14.0.1601"},
		{"v0.13.1592-a8056e9_os0.13.1592d-fcbc8b6-201808271434", "00.13.00", "0.13.0.1592"},
		{"v0.14.1627-12b32e4_os0.14.1627d-09827aa-201808302334", "00.14.00", "0.14.0.1627"},
		{"v0.14.284-25903f3_os0.14.284d-335cb47-201809101628", "00.14.00", "0.14.0.284"},
		{"v0.14.20-25903f3_os0.14.284d-335cb47-201809101628", "00.14.00", "0.14.0.20"},
		{"v0.14.0-5900c03_os0.14.0d-167f912-201809071708", "00.14.00", "0.14.0.0"},
		{"v0.13.1554-948c05c_os0.13.1554d-cb89682-201808202108", "00.13.00", "0.13.0.1554"},
		{"v0.13.157-7850c57_os0.13.1537d-195843b-201808161540", "00.13.00", "0.13.0.157"},
		{"v1.0.0.1234-7850c57_os1.0.0.1234d-195843b-201808161540", "01.00.00", "1.0.0.1234"},
		{"v0.14.1727-96196a9_os0.14.1727d-73ca2a8-201809240101", "00.14.00", "0.14.0.1727"},
		{"v1.0.1.1730-e38b110_os1.0.1.1730d-5abaf2d-201809292200", "01.00.01", "1.0.1.1730"},
	}

	for _, tc := range testCases {
		ver, dasVer := ExtractVersionBuildString(tc.fw)
		assert.Equal(t, tc.ver, ver)
		assert.Equal(t, tc.dasVer, dasVer)
	}
}

func TestDeconstructServiceHash(t *testing.T) {
	testCases := []struct {
		svc    string
		lang   string
		mode   string
		result pb.IntentService
	}{
		{"dialogflow", "en", "vc", pb.IntentService_DIALOGFLOW},
		{"df", "en", "vc", pb.IntentService_DIALOGFLOW},
		{"df", "en-us", "vc", pb.IntentService_DIALOGFLOW},
		{"dialogflow", "en-us", "vc", pb.IntentService_DIALOGFLOW},
		{"df", "fr", "vc", pb.IntentService_DIALOGFLOW},
		{"dialogflow", "fr-fr", "game", pb.IntentService_DIALOGFLOW},
		{"dialogflow", "de", "vc", pb.IntentService_DIALOGFLOW},
		{"lex", "en", "vc", pb.IntentService_LEX},
		{"lex", "en-us", "vc", pb.IntentService_LEX},
	}

	for _, tc := range testCases {
		svcHash, _ := GetServiceHashKey(tc.svc, tc.lang, tc.mode)
		res := DeconstructServiceHash(svcHash.Key)
		assert.Equal(t, tc.result, *res.PbService)
	}

	res := DeconstructServiceHash("blah_en-us_game")
	assert.Nil(t, res)

}
