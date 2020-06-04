package requestrouter

import (
	"context"
	"encoding/json"
	"errors"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/dynamodb"

	"github.com/anki/sai-awsutil/dynamoutil"
	"github.com/anki/sai-go-util/log"
)

const (
	// TableFWBotVersions
	// DynamoDB table containing maps of firmware version to intent-service version

	TableFWBotVersions = "fw_intent_versions"

	// AttributeHashKeyService, type S, primary key to identify service to use
	// e.g. "dialogflow_en-us," "lex_en_us"
	AttributeHashKeyService = "service"

	// AttributeRangeKeyFW, type S, sort key, robot firmware version e.g. "0.12"
	AttributeRangeKeyFW = "fw"

	// AttributeDialogflowProjName, type S, DF project name, e.g. "victor-dev-en-us"
	AttributeDialogflowProjName = "df_name"

	// AttributeDialogflowVersion, type S, DF env name, e.g. "df-prod-0_12"
	AttributeDialogflowVersion = "df_ver"

	// AttributeLexName, type S, Lex bot name, e.g. "victor-dev-en-us"
	AttributeLexName = "lex_name"

	// AttributeLexAlias, type S, Lex bot alias, e.g. "lex_prod_a_bc"
	AttributeLexAlias = "lex_ver"

	// AttributeCredentialKey, type S, DF credential key in Secrets Manager
	AttributeCredentialKey = "creds"

	// AttributeEnable, type BOOL, indicate if version is enabled.
	AttributeEnable = "enable"

	// AttributeUpdated, type S, format Y-M-D-H-M-S
	AttributeUpdated = "updated"

	// TableParameters
	// DynamoDB table storing the distribution ratio for intent-request to 3rd party intent services

	TableParameters = "parameters"

	// AttributeHashKeyParam, typeS, value = ParameterRatio (see below)
	AttributeHashKeyParam = "param"

	// ParameterRatio is the hash key value for storing intent routing ratio
	ParameterRatio = "intent-routing-ratio"

	// AttributeRangeKeyCreated, type S, format Y-M-D-H-M-S, ratio created datetime
	AttributeRangeKeyCreated = "created"

	// AttributeParamValue, type S, JSON string of ratio, e.g. "{\"df\": 90, \"lex\": 10, \"ms\": 0}"
	AttributeParamValue = "value"

	// AttributeCreatedBy, type S, source of the ratio update, could be account-id, or tool-name
	AttributeCreatedBy = "created_by"

	// MaxVersionItemsToEvaluate is the max. no. items to evaluate
	// (not necessarily the no. of matching items). We keep a maximum of 10 versions for each service.
	MaxVersionItemsToEvaluate = 10

	ErrorNoVersionItemFound = "No fw version DynamoDB item found"
	ErrorNoRatioItemFound   = "No routing ratio DynamoDB item found"
)

type RouteStore struct {
	db *dynamoutil.DB
}

func NewRouteStore(prefix string, cfg *aws.Config) (*RouteStore, error) {

	// table for firmware to intent versions mapping
	versionsMapTableSpec := &dynamoutil.TableSpec{
		Name: TableFWBotVersions,
		PrimaryKey: dynamoutil.KeySpec{
			KeyName:    AttributeHashKeyService,
			SubkeyName: AttributeRangeKeyFW,
			SubkeyType: dynamoutil.AttributeTypeString,
		},
	}

	// table for intent service routing ratios
	routeTableSpec := &dynamoutil.TableSpec{
		Name: TableParameters,
		PrimaryKey: dynamoutil.KeySpec{
			KeyName:    AttributeHashKeyParam,
			SubkeyName: AttributeRangeKeyCreated,
			SubkeyType: dynamoutil.AttributeTypeString,
		},
	}

	db, err := dynamoutil.NewDB(
		dynamoutil.WithPrefix(prefix+"_"),
		dynamoutil.WithConfig(cfg),
		dynamoutil.WithTable(versionsMapTableSpec),
		dynamoutil.WithTable(routeTableSpec),
	)

	if err != nil {
		return nil, err
	}
	return &RouteStore{db: db}, nil
}

//
// Helpers
//

func (s *RouteStore) CreateTables() error {
	return s.db.CreateTables()
}

func (s *RouteStore) CheckTables() error {
	alog.Info{"action": "check_tables", "tables": s.db.GetTable(TableFWBotVersions).Name}.Log()
	return s.db.CheckTables()
}

func (s *RouteStore) StoreVersion(ctx context.Context, version *FirmwareIntentVersion) error {
	item := toVersionItem(version)
	err := s.db.GetTable(TableFWBotVersions).SimplePutItem(ctx, item)
	if err != nil {
		alog.Error{
			"action":           "store_fw_version",
			"status":           alog.StatusFail,
			"error":            err,
			"service":          version.Service,
			"firmware_version": version.FwVersion,
			"intent_version":   version.IntentVersion.String(),
			"msg":              "Fail to store fw-intent version mapping",
		}.Log()
		return err
	}

	alog.Info{
		"action":           "store_fw_version",
		"status":           alog.StatusOK,
		"version":          version,
		"service":          version.Service,
		"firmware_version": version.FwVersion,
		"intent_version":   version.IntentVersion.String(),
	}.Log()

	return nil
}

// GetVersion gets the intent-version for a particular fw version
func (s *RouteStore) GetVersion(ctx context.Context, svcHashKey string, fwRangeKey string) (*FirmwareIntentVersion, error) {
	table := s.db.GetTable(TableFWBotVersions)

	sortKey := dynamoutil.SortKeyExpression{
		Type: dynamoutil.Equal,
		Value1: &dynamodb.AttributeValue{
			S: aws.String(fwRangeKey),
		},
	}

	queryOpts := []dynamoutil.QueryConfig{
		dynamoutil.WithCompoundKey(svcHashKey, sortKey),
		dynamoutil.WithSortDescending(),
	}

	printQueryOpts(table, queryOpts)

	out, err := table.Query(ctx, queryOpts...)
	if err != nil {
		alog.Error{"action": "query_version", "status": alog.StatusFail, "error": err}.Log()
		return nil, err
	}

	for _, item := range out.Items {
		fwMap, err := fromVersionItem(item)
		if err == nil {
			return fwMap, nil
		}
	}

	return nil, errors.New(ErrorNoVersionItemFound)
}

// GetLastEnabledVersion gets the closest *enabled* fw version to the fwSortKey
// e.g. 0.12 disabled, 0.11 enabled, 0.10 enabled, func will return 0.11 for fwSortKey=0.12
func (s *RouteStore) GetLastEnabledVersion(ctx context.Context, svcKey string, fwSortKey string) (*FirmwareIntentVersion, error) {
	table := s.db.GetTable(TableFWBotVersions)

	queryInput := &dynamodb.QueryInput{
		TableName:              aws.String(table.GetTableName()),
		Limit:                  aws.Int64(MaxVersionItemsToEvaluate),
		ScanIndexForward:       aws.Bool(false), // descending
		KeyConditionExpression: aws.String("#key = :v1 and #sortkey <= :v2"),
		FilterExpression:       aws.String("#en = :en"),
		ExpressionAttributeNames: map[string]*string{
			"#key":     aws.String("service"),
			"#sortkey": aws.String("fw"),
			"#en":      aws.String("enable"),
		},
		ExpressionAttributeValues: map[string]*dynamodb.AttributeValue{
			":v1": {S: aws.String(svcKey)},
			":v2": {S: aws.String(fwSortKey)},
			":en": {BOOL: aws.Bool(true)},
		},
	}

	alog.Debug{"action": "get_enabled_version", "query": queryInput}.Log()

	out, err := s.db.ExecuteQuery(ctx, queryInput)
	if err != nil {
		alog.Error{
			"action": "query_version_enable",
			"status": alog.StatusFail,
			"error":  err,
			"msg":    "Fail to query fw-version table",
		}.Log()
		return nil, err
	}

	for _, item := range out.Items {
		fwMap, err := fromVersionItem(item)
		if err != nil {
			alog.Error{
				"action": "query_version_enable",
				"status": alog.StatusError,
				"error":  err,
				"msg":    "Fail to convert dynamodb item to fw version map",
			}.Log()
			return nil, err
		} else {
			return fwMap, nil
		}
	}

	return nil, errors.New(ErrorNoVersionItemFound)
}

func (s *RouteStore) StoreRatio(ctx context.Context, ratio *Ratio) error {
	item := toRatioItem(ratio)
	err := s.db.GetTable(TableParameters).SimplePutItem(ctx, item)
	if err != nil {
		alog.Error{
			"action": "store_ratio",
			"status": alog.StatusFail,
			"error":  err,
			"ratio":  ratio,
			"msg":    "Fail to store routing ratio",
		}.Log()
		return err
	}

	alog.Info{
		"action": "store_ratio",
		"status": alog.StatusOK,
		"ratio":  ratio,
	}.Log()

	return nil
}

func (s *RouteStore) GetRecentRatios(ctx context.Context, n int) ([]*Ratio, error) {
	table := s.db.GetTable(TableParameters)

	queryOpts := []dynamoutil.QueryConfig{
		dynamoutil.WithSimpleKey(ParameterRatio),
		dynamoutil.WithLimit(int64(n)),
		dynamoutil.WithSortDescending(),
	}

	printQueryOpts(table, queryOpts)

	out, err := table.Query(ctx, queryOpts...)
	if err != nil {
		alog.Error{"action": "get_recent_ratios", "status": alog.StatusFail, "error": err}.Log()
		return nil, err
	}

	ratios := []*Ratio{}
	for _, item := range out.Items {
		r, err := fromRatioItem(item)
		if err == nil {
			alog.Debug{"action": "get_recent_ratios", "ratio": r}.Log()
			ratios = append(ratios, r)
		}
	}

	if len(ratios) == 0 {
		return nil, errors.New(ErrorNoRatioItemFound)
	}

	return ratios, nil
}

//
// helpers
//

// printQueryOpts outputs dynamodb QueryInput
func printQueryOpts(t *dynamoutil.TableSpec, opts []dynamoutil.QueryConfig) {
	in := &dynamodb.QueryInput{
		TableName: aws.String(t.GetTableName()),
	}

	for _, opt := range opts {
		opt(t, in)
	}
	alog.Debug{
		"action": "query_configs",
		"opts":   in.String(),
		"table":  t,
	}.Log()
}

// toVersionItem (table fw_intent_versions) converts a FirmwareVersionsMap struct to a dynamodb item
func toVersionItem(v *FirmwareIntentVersion) dynamoutil.Item {
	return dynamoutil.NewItem().
		SetStringValue(AttributeHashKeyService, v.Service).
		SetStringValue(AttributeRangeKeyFW, v.FwVersion).
		SetStringValueIfNotEmpty(AttributeDialogflowProjName, v.IntentVersion.DFProjectName).
		SetStringValueIfNotEmpty(AttributeDialogflowVersion, v.IntentVersion.DFVersion).
		SetStringValueIfNotEmpty(AttributeCredentialKey, v.IntentVersion.DFCredsKey).
		SetStringValueIfNotEmpty(AttributeLexName, v.IntentVersion.LexBotName).
		SetStringValueIfNotEmpty(AttributeLexAlias, v.IntentVersion.LexBotAlias).
		SetBoolValue(AttributeEnable, v.Enable).
		SetStringValue(AttributeUpdated, GetTimeStamp())
}

// fromVersionItem (table fw_intent_versions) converts item to a FirmwareVersionsMap struct
func fromVersionItem(item dynamoutil.Item) (*FirmwareIntentVersion, error) {
	svc := item.GetStringValue(AttributeHashKeyService)
	deconstructed := DeconstructServiceHash(svc)
	if deconstructed == nil {
		return nil, errors.New("Cannot retrieve intent-service")
	}

	svcVer := IntentServiceVersion{
		IntentSvc:     *deconstructed.PbService,
		DFProjectName: item.GetStringValue(AttributeDialogflowProjName),
		DFVersion:     item.GetStringValue(AttributeDialogflowVersion),
		DFCredsKey:    item.GetStringValue(AttributeCredentialKey),
		LexBotName:    item.GetStringValue(AttributeLexName),
		LexBotAlias:   item.GetStringValue(AttributeLexAlias),
	}

	versionMap := &FirmwareIntentVersion{
		FwVersion:     item.GetStringValue(AttributeRangeKeyFW),
		Service:       svc,
		Lang:          deconstructed.LangString,
		Mode:          deconstructed.ModeString,
		Enable:        item.GetBoolValue(AttributeEnable),
		IntentVersion: svcVer,
	}
	return versionMap, nil
}

// toRatioItem (table parameters) converts a Ratio struct to parameters table item
func toRatioItem(r *Ratio) dynamoutil.Item {
	ratio, _ := json.Marshal(r.Value)
	alog.Debug{"action": "json_ratio", "ratio": string(ratio)}.Log()

	return dynamoutil.NewItem().
		SetStringValue(AttributeHashKeyParam, ParameterRatio).
		SetStringValue(AttributeRangeKeyCreated, GetTimeStamp()).
		SetStringValue(AttributeParamValue, string(ratio)).
		SetStringValue(AttributeCreatedBy, r.CreatedBy)
}

// fromRatioItem (table parameters) converts table item to Ratio struct
func fromRatioItem(item dynamoutil.Item) (*Ratio, error) {
	var ratioVal RatioValue
	ratioBytes := []byte(item.GetStringValue(AttributeParamValue))
	if err := json.Unmarshal(ratioBytes, &ratioVal); err != nil {
		return nil, err
	}
	return &Ratio{
		Date:      item.GetStringValue(AttributeRangeKeyCreated),
		Value:     ratioVal,
		CreatedBy: item.GetStringValue(AttributeCreatedBy),
	}, nil
}
