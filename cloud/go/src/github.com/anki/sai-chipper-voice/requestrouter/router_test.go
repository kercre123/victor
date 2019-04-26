package requestrouter

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"

	"github.com/anki/sai-go-util/dockerutil"
	"github.com/anki/sai-go-util/log"

	"fmt"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
)

type RequestRouterSuite struct {
	dockerutil.DynamoSuite
	router *RequestRouter
	ctx    context.Context
}

func init() {
	alog.ToStdout()
	alog.SetLevelByString("DEBUG")
}

func TestRequestRouterSuite(t *testing.T) {
	suite.Run(t, new(RequestRouterSuite))
}

var versions = map[string]*FirmwareIntentVersion{
	// Dialogflow versions
	"df-test-vc-00_08_00": CreateVersion(
		"dialogflow", "en-us", "vc", "00.08.00",
		"victor-test", "df-test-vc-00_08_00", "df_test_creds",
		"", "", false,
	),
	"df-test-vc-00_12_00": CreateVersion(
		"dialogflow", "en-us", "vc", "00.12.00",
		"victor-test", "df-test-vc-00_12_00", "df_test_creds",
		"", "", true,
	),
	"df-test-vc-00_13_00": CreateVersion(
		"dialogflow", "en-us", "vc", "00.00.13",
		"victor-test", "df-test-vc-00_13_00", "df_test_creds",
		"", "", false,
	), // disabled
	"df-test-vc-00_16_00": CreateVersion(
		"dialogflow", "en-us", "vc", "00.16.00",
		"victor-test", "df-test-vc-00_16_00", "df_test_creds",
		"", "", true,
	),
	"df-test-ga-00_15_00": CreateVersion(
		"dialogflow", "en-us", "game", "00.15.00",
		"victor-game-test", "df-test-ga-00_15_00", "df_game_test_creds",
		"", "", true,
	),

	// Lex versions
	"lex_test_vc_aa_bb_aa": CreateVersion(
		"lex", "en-us", "vc", "00.11.00", "", "", "",
		"victor-lex-test", "lex_test_vc_aa_bb_aa", true,
	),
	"lex_test_ga_aa_bb_aa": CreateVersion(
		"lex", "en-us", "game", "00.11.00", "", "", "",
		"victor-lex-game-test", "lex_test_ga_aa_bb_aa", true,
	),
	"lex_test_vc_aa_bd_aa": CreateVersion(
		"lex", "en-us", "vc", "00.14.00", "", "", "",
		"victor-lex-test", "lex_test_vc_aa_bd_aa", true,
	),
	"lex_test_vc_ab_ab_aa": CreateVersion(
		"lex", "en-us", "vc", "01.01.00", "", "", "",
		"victor-lex-test", "lex_test_vc_ab_ab_aa", true,
	),
}

var ratioValues = []RatioValue{
	{70, 30, 0},
	{5, 95, 0},
	{50, 50, 0},
}

var sleepInterval = time.Millisecond * 500

func (s *RequestRouterSuite) SetupTest() {
	s.ctx = context.Background()
	s.DynamoSuite.DeleteAllTables()
	s.router = New("test-", &sleepInterval, s.DynamoSuite.Config)
	s.router.Init(true, true)
	if s.router.Store != nil {
		if s.router.Store.CreateTables() != nil {
			s.T().Fatalf("Failed to initialize dynamodb tables")
		}

		// save some versions
		for _, v := range versions {
			s.router.Store.StoreVersion(s.ctx, v)
		}

		// save a ratio
		r := ratioValues[0]
		ratio, _ := NewRatio(r.Dialogflow, r.Lex, r.MS, "test")
		s.router.Store.StoreRatio(s.ctx, ratio)

	}
	s.router.Start()
	<-s.router.Ready()
}

func (s *RequestRouterSuite) TearDownSuite() {
	s.router.Stop()
	s.DynamoSuite.TearDownSuite()
}

func (s *RequestRouterSuite) TestLoop() {
	t := s.T()
	sleepTime := time.Millisecond * 300

	t.Log("Router should be running")
	assert.True(t, s.router.isRunning)
	assert.Equal(t, ratioValues[0].Dialogflow, s.router.Ratio.Value.Dialogflow)

	t.Log("Update ratio")
	r := ratioValues[2]
	ratio, _ := NewRatio(r.Dialogflow, r.Lex, r.MS, "test2")
	s.router.Store.StoreRatio(s.ctx, ratio)

	// wait for router to update ratio
	time.Sleep(sleepInterval + sleepTime)

	t.Log("Check ratio again")
	assert.True(t, s.router.isRunning)
	assert.Equal(t, r.Dialogflow, s.router.Ratio.Value.Dialogflow)

	t.Log("Stop router")
	s.router.Stop()

	time.Sleep(sleepTime)

	t.Log("Verify that router has stopped")
	assert.False(t, s.router.isRunning)

	time.Sleep(sleepInterval)

	t.Log("Reset ratio to initial")
	r = ratioValues[0]
	ratio, _ = NewRatio(r.Dialogflow, r.Lex, r.MS, "test")
	s.router.Store.StoreRatio(s.ctx, ratio)

	t.Log("Restart router and check ratio")
	s.router.Start()
	time.Sleep(sleepTime)

	assert.Equal(t, r.Dialogflow, s.router.Ratio.Value.Dialogflow)

}

func (s *RequestRouterSuite) TestDefaultVersions() {
	t := s.T()
	version := CreateDefaultVersions(
		"df-project", "df-project-creds",
		"df-games-project", "df-games-project-creds",
		"lex_bot", "lex_games_bot",
	)
	s.router.Init(true, true, version...)
	input := RequestRouteInput{
		Requested: pb.IntentService_DEFAULT,
		LangCode:  pb.LanguageCode_ENGLISH_US,
		Mode:      pb.RobotMode_VOICE_COMMAND,
		RobotId:   "10",
		FwVersion: "00.00.00",
	}

	routed, err := s.router.Route(s.ctx, input)
	assert.NoError(t, err)
	assert.Equal(t, pb.IntentService_DIALOGFLOW, routed.IntentVersion.IntentSvc)
	assert.Equal(t, DefaultDialogflowVersion, routed.IntentVersion.DFVersion)

	input.RobotId = "50" // should get routed to Lex
	routed, err = s.router.Route(s.ctx, input)
	assert.NoError(t, err)
	assert.Equal(t, pb.IntentService_LEX, routed.IntentVersion.IntentSvc)
	assert.Equal(t, DefaultLexAlias, routed.IntentVersion.LexBotAlias)

}

func (s *RequestRouterSuite) TestDefaultVersionsNoLex() {
	t := s.T()
	version := CreateDefaultVersions(
		"df-project", "df-project-creds",
		"df-games-project", "df-games-project-creds",
		"lex_bot", "lex_games_bot",
	)
	s.router.Init(true, false, version...)
	input := RequestRouteInput{
		Requested: pb.IntentService_DEFAULT,
		LangCode:  pb.LanguageCode_ENGLISH_US,
		Mode:      pb.RobotMode_VOICE_COMMAND,
		RobotId:   "10",
		FwVersion: "00.00.00",
	}

	routed, err := s.router.Route(s.ctx, input)
	assert.NoError(t, err)
	assert.Equal(t, pb.IntentService_DIALOGFLOW, routed.IntentVersion.IntentSvc)
	assert.Equal(t, DefaultDialogflowVersion, routed.IntentVersion.DFVersion)

	input.RobotId = "50" // id should route to Lex, but since it's not enabled, we get dialogflow again
	routed, err = s.router.Route(s.ctx, input)
	assert.NoError(t, err)
	assert.Equal(t, pb.IntentService_DIALOGFLOW, routed.IntentVersion.IntentSvc)

}

func (s *RequestRouterSuite) TestRoute() {
	t := s.T()

	dialogflowId := "10" // robotid is Hex, decimal = 16
	lexId := "50"        // robotid is Hex, decimal = 80

	// Success Cases
	testCases := []struct {
		input  RequestRouteInput
		result string
	}{
		// #0: we have a valid 0.12 (aka 0.12.0) DF version
		{
			input: RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_VOICE_COMMAND,
				dialogflowId,
				"00.12.00"},
			result: "df-test-vc-00_12_00",
		},
		// #1: 0.13 version is disabled,
		// the closest valid version for 0.14 is 0.12
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_VOICE_COMMAND,
				dialogflowId,
				"00.14.00"},
			"df-test-vc-00_12_00",
		},
		// #2: closest valid game version is 0.15
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_GAME,
				dialogflowId,
				"00.16.15"},
			"df-test-ga-00_15_00",
		},
		// #3: closest valid vc version is 0.16
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_VOICE_COMMAND,
				dialogflowId,
				"01.01.00"},
			"df-test-vc-00_16_00",
		},
		// #4: closest valid vc version is 0.16
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_VOICE_COMMAND,
				dialogflowId,
				"01.99.01"},
			"df-test-vc-00_16_00",
		},

		// #5: lex
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_VOICE_COMMAND,
				lexId,
				"00.14.00"},
			"lex_test_vc_aa_bd_aa",
		},
		// #6:
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_VOICE_COMMAND,
				lexId,
				"01.14.00"},
			"lex_test_vc_ab_ab_aa",
		},
		// #7:
		{
			RequestRouteInput{
				pb.IntentService_DEFAULT,
				pb.LanguageCode_ENGLISH_US,
				pb.RobotMode_GAME,
				lexId,
				"00.14.00"},
			"lex_test_ga_aa_bb_aa",
		},
	}

	t.Log("Success cases")
	for i, tc := range testCases {
		res, err := s.router.Route(s.ctx, tc.input)
		assert.NoError(t, err)
		t.Log(fmt.Sprintf("%d. query: %s, version: %s, %v, %v", i, tc.input.FwVersion, res.String(), s.router.LexEnabled, s.router.DialogflowEnabled))

		assert.Equal(t, versions[tc.result].IntentVersion, res.IntentVersion)
		t.Log(fmt.Sprintf("%d. query: %s, version: %s", i, tc.input.FwVersion, res.String()))

	}

	// Error Cases
	testCasesErr := []struct {
		input  RequestRouteInput
		result string
	}{
		// version doesn't exist
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND,
			dialogflowId,
			"00.09.00"}, "",
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_GAME,
			dialogflowId,
			"00.11.00"}, "",
		},
		// version doesn't exist
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND,
			lexId,
			"00.09.00"}, "",
		},
		// version string incorrect
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND,
			dialogflowId,
			"0.09.0"}, "",
		},
	}

	// no default version set, so we're getting errors
	t.Log("Error cases")
	for i, tc := range testCasesErr {
		res, err := s.router.Route(s.ctx, tc.input)
		assert.Error(t, err)
		if res != nil {
			t.Log(fmt.Sprintf("error case but got result %s", res.String()))
		}
		t.Log(fmt.Sprintf("%d. query: %s, err: %s", i, tc.input.FwVersion, err.Error()))

	}

	// now, set a default for Dialogflow, Voice-command, en-us
	defaultVersion := CreateVersion(
		GetPbServiceName(pb.IntentService_DIALOGFLOW),
		"en-us",
		"vc",
		DefaultFWString,
		"victor-test",
		"dialogflow-vc-default",
		"victor-test-creds",
		"", "", // nothing for lex
		true)

	t.Log("Add Dialogflow, voice-command, en-us default")
	s.router.Init(true, true, *defaultVersion)

	// re-run error cases, we should be getting default for DF, voice-command
	t.Log("Error cases should get some default versions")
	for i, tc := range testCasesErr {
		res, err := s.router.Route(s.ctx, tc.input)
		if tc.input.RobotId == dialogflowId && tc.input.Mode == pb.RobotMode_VOICE_COMMAND {
			assert.NoError(t, err)
			assert.Equal(t, defaultVersion.IntentVersion, res.IntentVersion)
			t.Log(
				fmt.Sprintf("%d. query: %s, id: %s, version: %s",
					i,
					tc.input.FwVersion,
					tc.input.RobotId,
					res.String(),
				),
			)

		} else {
			// no defaults for lex or game mode
			assert.Error(t, err)
			t.Log(fmt.Sprintf("%d. query: %s, no default for lex or game", i, tc.input.FwVersion))
		}
	}

	// switch ratio so all goes to Lex
	r := ratioValues[1]
	ratio, _ := NewRatio(r.Dialogflow, r.Lex, r.MS, "test2")
	s.router.Store.StoreRatio(s.ctx, ratio)
	time.Sleep(sleepInterval)
	t.Log(fmt.Sprintf("Switch ratio to df: %d, lex: %d", r.Dialogflow, r.Lex))

	// retry all ok cases, should all go to Lex
	expected := []string{
		"lex_test_vc_aa_bb_aa",
		"lex_test_vc_aa_bd_aa",
		"lex_test_ga_aa_bb_aa",
		"lex_test_vc_ab_ab_aa",
		"lex_test_vc_ab_ab_aa",
		"lex_test_vc_aa_bd_aa",
		"lex_test_vc_ab_ab_aa",
		"lex_test_ga_aa_bb_aa",
	}

	t.Log("Dialogflow vc should all be mapped to lex now")
	for i, tc := range testCases {
		t.Log(fmt.Sprintf("%d. query: %s", i, tc.input.FwVersion))
		res, err := s.router.Route(s.ctx, tc.input)
		assert.NoError(t, err)
		assert.Equal(t, versions[expected[i]].IntentVersion, res.IntentVersion)
	}

}

func TestSleepInterval(t *testing.T) {

	interval := getSleepInterval()
	correct := false
	if interval >= minSleepInterval && interval <= (minSleepInterval+sleepIntervalRange) {
		correct = true
	}
	t.Log("Interval ", interval, time.Now())
	assert.True(t, correct)
}

func TestChooseIntentService(t *testing.T) {
	ratioVal := RatioValue{70, 30, 0}
	testCases := []struct {
		input RequestRouteInput
		ratio RatioValue
		svc   pb.IntentService
	}{
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND, "10", "00.12.00"}, // robotid is Hex, decimal = 16
			ratioVal, pb.IntentService_DIALOGFLOW, // robotid maps to DF
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND, "50", "00.12.00"}, // robotid decimal = 80
			ratioVal, pb.IntentService_LEX, // robotid maps to lex
		},
		{RequestRouteInput{
			pb.IntentService_DIALOGFLOW,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND, "118", "00.13.00"}, // robotid decimal = 180
			ratioVal, pb.IntentService_DIALOGFLOW, // request specify DF, ignore robotid mapping
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_FRENCH,
			pb.RobotMode_VOICE_COMMAND, "50", "00.12.12"}, // robotid decimal = 80
			ratioVal, pb.IntentService_DIALOGFLOW, // lang is french, so DF
		},
		{RequestRouteInput{
			pb.IntentService_LEX,
			pb.LanguageCode_FRENCH,
			pb.RobotMode_VOICE_COMMAND, "50", "00.00.12"}, // robotid decimal = 80
			ratioVal, pb.IntentService_DIALOGFLOW, // lang is french, ignore request specification
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_AU,
			pb.RobotMode_GAME, "50", "00.12.00"}, // robotid decimal = 80
			ratioVal, pb.IntentService_DIALOGFLOW, // not en-us, game, robotid map to DF
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_GAME, "50", "00.12.00"}, // robotid decimal = 80
			ratioVal, pb.IntentService_LEX, // game, robotid map to Lex
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_GAME, "46", "00.12.00"}, // robotid decimal = 70
			ratioVal, pb.IntentService_LEX, // robotid mapping to 70, which maps to lex
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_GAME, "45", "00.12.00"}, // robotid decimal = 69
			ratioVal, pb.IntentService_DIALOGFLOW, // robotid mapping 69 < 70, so DF
		},
	}

	for _, tc := range testCases {
		svc := chooseIntentService(context.Background(), tc.input, tc.ratio, true, true)
		t.Log("robotid: "+tc.input.RobotId, " -> ", svc)
		assert.Equal(t, tc.svc, svc)
	}
}

func TestChooseIntentServiceLexDisabled(t *testing.T) {
	ratioVal := RatioValue{70, 30, 0}
	testCases := []struct {
		input RequestRouteInput
		ratio RatioValue
		svc   pb.IntentService
	}{
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_VOICE_COMMAND, "50", "00.12.00"}, // robotid decimal = 80
			ratioVal, pb.IntentService_DIALOGFLOW, // robotid maps to lex but it's disabled
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_GAME, "50", "00.12.00"}, // robotid decimal = 80
			ratioVal, pb.IntentService_DIALOGFLOW, // game, robotid map to Lex
		},
		{RequestRouteInput{
			pb.IntentService_DEFAULT,
			pb.LanguageCode_ENGLISH_US,
			pb.RobotMode_GAME, "46", "00.12.00"}, // robotid decimal = 70
			ratioVal, pb.IntentService_DIALOGFLOW, // robotid mapping to 70, which maps to lex
		},
	}

	for _, tc := range testCases {
		svc := chooseIntentService(context.Background(), tc.input, tc.ratio, true, false)
		t.Log("robotid: "+tc.input.RobotId, " -> ", svc)
		assert.Equal(t, tc.svc, svc)
	}
}
