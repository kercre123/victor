package requestrouter

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/stretchr/testify/suite"

	"github.com/anki/sai-go-util/dockerutil"
)

type RouteStoreSuite struct {
	dockerutil.DynamoSuite
	Store *RouteStore
	ctx   context.Context
}

func TestRouteStoreSuite(t *testing.T) {
	suite.Run(t, new(RouteStoreSuite))
}

func (s *RouteStoreSuite) SetupTest() {
	s.DynamoSuite.DeleteAllTables()
	if store, err := NewRouteStore("test-", s.DynamoSuite.Config); err != nil {
		s.T().Fatalf("Failed to initialize route store: %s", err)
	} else {
		s.Store = store
		if err = s.Store.CreateTables(); err != nil {
			s.T().Fatalf("Failed to initialize dynamodb tables: %s", err)
		}
	}
	s.ctx = context.Background()
}

func (s *RouteStoreSuite) TestCreateTables() {
	require.NoError(s.T(), s.Store.db.CheckTables())
	require.Len(s.T(), s.ListTableNames(), 2)
}

func (s *RouteStoreSuite) TestStoreVersion() {
	t := s.T()
	ctx := context.Background()

	fw := "0.13"
	normFw, _ := NormalizeFwVersion(fw)
	dfVer := "df-test-vc-00_00_13"
	fwVer := CreateVersion("dialogflow", "en", "vc", normFw, "victor-test", dfVer, "df-creds", "", "", true)
	assert.NotNil(t, fwVer)

	// Store
	require.NoError(t, s.Store.StoreVersion(ctx, fwVer))

	// Get
	out, err := s.Store.GetVersion(ctx, fwVer.Service, normFw)
	require.NoError(t, err)

	assert.Equal(t, normFw, out.FwVersion)
	assert.Equal(t, dfVer, out.IntentVersion.DFVersion)
}

type enableTest struct {
	svc    string
	fw     string
	ver    string
	enable bool
}

func (s *RouteStoreSuite) TestGetEnabledVersion() {
	t := s.T()
	ctx := context.Background()

	testCases := []enableTest{
		{"df", "00.00.10", "dialogflow-testenv-00_00_10", true},
		{"df", "00.00.11", "dialogflow-testenv-00_00_11", true},
		{"df", "00.00.14", "dialogflow-testenv-00_00_14", false},
		{"df", "00.00.18", "dialogflow-testenv-00_00_18", true},
		{"lex", "00.00.10", "lex_testenv_aa_aa_ba", true},
		{"lex", "00.00.11", "lex_testenv_aa_aa_bb", false},
	}

	for _, tc := range testCases {
		var fwVer *FirmwareIntentVersion
		if tc.svc == "df" {
			fwVer = CreateVersion(tc.svc, "en", "vc", tc.fw, "vic-test", tc.ver, "creds", "", "", tc.enable)
		} else {
			fwVer = CreateVersion(tc.svc, "en", "vc", tc.fw, "", "", "", "vic-test", tc.ver, tc.enable)
		}
		t.Log(fwVer)
		assert.NotNil(t, fwVer)

		require.NoError(t, s.Store.StoreVersion(ctx, fwVer))
	}

	// Test Dialogflow
	svcKey := "dialogflow_en-us_vc"

	queryFW := "00.00.16" // this version doesn't exist
	expectsFW := "00.00.11"
	out, err := s.Store.GetVersion(ctx, svcKey, queryFW) // should fail
	require.Error(t, err)

	t.Log("Get Dialogflow last enabled version <= " + queryFW)
	out, err = s.Store.GetLastEnabledVersion(ctx, svcKey, queryFW) // should pass
	require.NoError(t, err)
	assert.Equal(t, expectsFW, out.FwVersion)

	queryFW = "00.00.18"
	expectsFW = "00.00.18"
	t.Log("GetDialogflow last enabled version <= " + queryFW)
	out, err = s.Store.GetLastEnabledVersion(ctx, svcKey, queryFW)
	require.NoError(t, err)
	assert.Equal(t, expectsFW, out.FwVersion)

	// Test Lex
	svcKey = "lex_en-us_vc"

	queryFW = "00.00.16" // this version doesn't exist
	expectsFW = "00.00.10"
	t.Log("Get Lex last enabled version <= " + queryFW)
	out, err = s.Store.GetLastEnabledVersion(ctx, svcKey, queryFW) // should pass
	require.NoError(t, err)
	assert.Equal(t, expectsFW, out.FwVersion)

	queryFW = "00.00.09" // no version equal or lower than this exist, should fail
	out, err = s.Store.GetLastEnabledVersion(ctx, svcKey, queryFW)
	require.Error(t, err)
}

func (s *RouteStoreSuite) TestStoreRatio() {
	t := s.T()
	ctx := context.Background()

	// Fail ratio check
	r1, err := NewRatio(99, 10, 0, "tester")
	require.Error(t, err)
	require.Nil(t, r1)

	// this is a valid ratio
	r2, err := NewRatio(90, 10, 0, "tester")
	require.NoError(t, err)

	// store r2
	err = s.Store.StoreRatio(ctx, r2)
	require.NoError(t, err)

	// Get r2 back
	outputs, err := s.Store.GetRecentRatios(ctx, 2)
	require.NoError(t, err)
	assert.Len(t, outputs, 1, "Number of retrieved ratios should be 1")
	assert.Equal(t, r2.Value.Dialogflow, outputs[0].Value.Dialogflow)

	// Store another ratio r3
	r3, err := NewRatio(80, 20, 0, "tester2")
	require.NoError(t, err)

	err = s.Store.StoreRatio(ctx, r3)
	require.NoError(t, err)

	// Get all ratios back
	outputs, err = s.Store.GetRecentRatios(ctx, 2)
	require.NoError(t, err)
	assert.Len(t, outputs, 2, "Number of retrieved ratios should be 2")
	assert.Equal(t, r3.Value.Dialogflow, outputs[0].Value.Dialogflow)
	assert.Equal(t, r3.CreatedBy, outputs[0].CreatedBy)
	assert.Equal(t, r2.Value.Dialogflow, outputs[1].Value.Dialogflow)
	assert.Equal(t, r2.CreatedBy, outputs[1].CreatedBy)
}
