package requestrouter

import (
	"errors"
	"fmt"
	pb "github.com/anki/sai-chipper-voice/proto/anki/chipperpb"
	"github.com/anki/sai-go-util/log"
	"regexp"
	"strconv"
	"strings"
)

type FirmwareIntentVersion struct {
	// Service is the hash key value [svc]_[lang]_[mode]. e.g. "dialogflow_en-us_vc"
	Service       string
	FwVersion     string
	Lang          string
	Mode          string
	IntentVersion IntentServiceVersion
	Enable        bool
}

type ServiceHash struct {
	Key        string
	LangString string
	ModeString string
	PbService  *pb.IntentService
}

var ValidIntentServices = map[string]pb.IntentService{
	"dialogflow": pb.IntentService_DIALOGFLOW,
	"df":         pb.IntentService_DIALOGFLOW,
	"lex":        pb.IntentService_LEX,
	"ms":         pb.IntentService_BING_LUIS,
	"bing_luis":  pb.IntentService_BING_LUIS,
}

var ValidLanguages = map[string]string{
	"en":    "en-us",
	"en-us": "en-us",
	"gb":    "en-gb",
	"en-gb": "en-gb",
	"au":    "en-au",
	"en-au": "en-au",
	"de":    "de",
	"de-de": "de",
	"fr":    "fr-fr",
	"fr-fr": "fr-fr",
}

var LangCodeName = map[pb.LanguageCode]string{
	pb.LanguageCode_ENGLISH_US: "en-us",
	pb.LanguageCode_ENGLISH_AU: "en-au",
	pb.LanguageCode_ENGLISH_UK: "en-gb",
	pb.LanguageCode_FRENCH:     "fr-fr",
	pb.LanguageCode_GERMAN:     "de",
}

var ValidModes = map[string]pb.RobotMode{
	"vc":            pb.RobotMode_VOICE_COMMAND,
	"voice-command": pb.RobotMode_VOICE_COMMAND,
	"voice":         pb.RobotMode_VOICE_COMMAND,
	"game":          pb.RobotMode_GAME,
	"games":         pb.RobotMode_GAME,
}

// ModeShortName is added to the service hash key
var ModeShortName = map[pb.RobotMode]string{
	pb.RobotMode_VOICE_COMMAND: "vc",
	pb.RobotMode_GAME:          "game",
}

var (
	ErrorInvalidLexHash     = errors.New("Invalid lang for Lex, en-us only")
	ErrorInvalidServiceHash = errors.New("Invalid intent service")
	ErrorInvalidLanguage    = errors.New("Invalid language")
	ErrorInvalidRobotMode   = errors.New("Invalid robot mode")
	ErrorFWStringFormat     = errors.New("Firmware version string format is incorrect")
)

// ToLogValues provide fields we want to log
func (fw *FirmwareIntentVersion) ToLogValues() map[string]interface{} {
	ver := fw.IntentVersion.DFVersion
	bot := fw.IntentVersion.DFProjectName
	if fw.IntentVersion.IntentSvc == pb.IntentService_LEX {
		ver = fw.IntentVersion.LexBotAlias
		bot = fw.IntentVersion.LexBotName
	}

	return map[string]interface{}{
		"_key_hash":    fw.Service,
		"_key_sort_fw": fw.FwVersion,
		"_bot_name":    bot,
		"_bot_ver":     ver,
	}
}

func (fw *FirmwareIntentVersion) String() string {
	if fw != nil {
		ver := fw.IntentVersion.DFVersion
		bot := fw.IntentVersion.DFProjectName
		if fw.IntentVersion.IntentSvc == pb.IntentService_LEX {
			ver = fw.IntentVersion.LexBotAlias
			bot = fw.IntentVersion.LexBotName
		}

		return fmt.Sprintf(
			"svc_key: %s, fw_sortkey: %s, bot: %s, intent_ver: %s",
			fw.Service, fw.FwVersion, bot, ver)
	}
	return ""
}

func CreateVersion(
	// hash key info. note: fw normalised format is xx.xx.xx
	svc, lang, mode, fw string,
	// Dialogflow
	dialogflowProj, dialogflowVersion, dialogflowCreds string,
	// Lex
	lexBot, lexAlias string,
	enable bool,
) *FirmwareIntentVersion {

	svcHash, err := GetServiceHashKey(svc, lang, mode)
	if err != nil {
		alog.Error{
			"action": "create_version",
			"status": alog.StatusError,
			"error":  err,
			"msg": fmt.Sprintf("Fail to create version for svc=%s, lang=%s, mode=%s",
				svc, lang, mode),
		}.Log()
		return nil
	}

	intentVersion := &IntentServiceVersion{
		IntentSvc:     *svcHash.PbService,
		DFProjectName: dialogflowProj,
		DFVersion:     dialogflowVersion,
		DFCredsKey:    dialogflowCreds,
		LexBotName:    lexBot,
		LexBotAlias:   lexAlias,
	}

	// allow empty version string for Dialogflow default fw
	if *svcHash.PbService != pb.IntentService_DIALOGFLOW || fw != DefaultFWString {
		if err := intentVersion.Validate(); err != nil {
			alog.Error{
				"action": "create_version",
				"status": alog.StatusError,
				"error":  err,
				"msg":    "Fail to create intent version",
			}.Log()
			return nil
		}
	}

	return &FirmwareIntentVersion{
		Service:       svcHash.Key,
		Lang:          svcHash.LangString,
		Mode:          svcHash.ModeString,
		Enable:        enable,
		FwVersion:     fw,
		IntentVersion: *intentVersion,
	}
}

// GetServiceHashKey
// takes the intent service name (e.g. dialogflow), lang (e.g. en-us), and mode (e.g. game)
// and returns the hash key for fw_intent_versions table, and the correct pb.IntentService value
func GetServiceHashKey(intentSvc, lang, mode string) (*ServiceHash, error) {
	langString, ok := ValidLanguages[lang]
	if !ok {
		alog.Warn{
			"action": "get_service_hash",
			"status": alog.StatusError,
			"error":  "invalid language code",
			"lang":   lang,
		}.Log()
		return nil, ErrorInvalidLanguage
	}

	pbMode, ok := ValidModes[mode]
	if !ok {
		alog.Warn{
			"action": "get_service_hash",
			"status": alog.StatusError,
			"error":  "invalid robot mode",
			"mode":   mode,
		}.Log()
		return nil, ErrorInvalidRobotMode

	}

	modeName := ModeShortName[pbMode]
	if s, ok := ValidIntentServices[intentSvc]; ok {
		if langString != "en-us" && s != pb.IntentService_DIALOGFLOW {
			return nil, ErrorInvalidLexHash
		}
		svcHashKey := getKeyValue(s, langString, modeName)
		alog.Debug{
			"action": "get_service_hash",
			"hash":   svcHashKey,
			"lang":   langString,
			"mode":   modeName,
			"svc":    s,
		}.Log()
		return &ServiceHash{
			Key:        svcHashKey,
			LangString: langString,
			ModeString: modeName,
			PbService:  &s,
		}, nil
	}
	return nil, ErrorInvalidServiceHash
}

// GetPbServiceName returns the protobuf intent-service name in lower-case
func GetPbServiceName(s pb.IntentService) string {
	return strings.ToLower(pb.IntentService_name[int32(s)])
}

// getKeyValue returns the hash key for table fw_intent_versions
func getKeyValue(s pb.IntentService, lang, mode string) string {
	return fmt.Sprintf("%s_%s_%s", GetPbServiceName(s), lang, mode)
}

// DeconstructServiceHash
// converts "dialogflow_en-us" to pb.IntentService_DIALOGFLOW
func DeconstructServiceHash(s string) *ServiceHash {
	if s != "" {
		parts := strings.Split(s, "_")
		if len(parts) == 3 {
			if intentSvc, ok := ValidIntentServices[parts[0]]; ok {
				alog.Debug{"action": "deconstruct_hash",
					"hash": s,
					"svc":  intentSvc}.Log()

				return &ServiceHash{
					Key:        s,
					LangString: parts[1],
					ModeString: parts[2],
					PbService:  &intentSvc,
				}
			}
		}
	}
	return nil
}

// NormalizeFwVersion checks if the fw string is what we expects and normalize it to xx.xx.xx
// 1.0.13 to "01.00.13"
// 0.14 to "00.14.00"
func NormalizeFwVersion(fw string) (string, error) {
	parts := strings.Split(fw, ".")
	if len(parts) < 2 || len(parts) > 3 {
		return "", ErrorFWStringFormat
	}

	normalized := ""
	for _, p := range parts {
		num, err := strconv.Atoi(p)
		if num >= 100 {
			return "", ErrorFWStringFormat
		}
		if err != nil {
			return "", ErrorFWStringFormat
		}
		normalized += fmt.Sprintf("%02d.", num)
	}
	// remove the last "."
	newFw := normalized[:len(normalized)-1]

	// special case for dev string such as "0.12", we need to add ".00"
	if len(parts) == 2 {
		newFw += ".00"
	}

	alog.Debug{
		"action": "normalize_fw_string",
		"org":    fw,
		"norm":   newFw,
	}.Log()

	return newFw, nil
}

var fwRegex = regexp.MustCompile(`(\d\d\.\d\d\.\d\d)`)

func validFwString(fw string) bool {
	res := fwRegex.FindAllString(fw, -1)
	if len(res) == 0 || res[0] != fw {
		return false
	}
	return true
}

// getDasRobotVersion
func getDasRobotVersion(ver, buildNumber string, isDevVersion bool) string {
	version := ver
	if isDevVersion {
		// dev version, add <patch_number>
		version += ".0"
	}
	return version + "." + buildNumber
}

// "v0.14.1234-whatever" returns [v0.14.1234- v 0.14 1234 -]
var devVerRegex = regexp.MustCompile(`(^v)(\d+\.\d+)\.(\d+)(-)`)

// "v1.0.1.1234-whatever" returns [v1.0.0.1234- v 1.0.0 1234 -]
var prodVerRegex = regexp.MustCompile(`(^v)(\d+\.\d+\.\d+)\.(\d+)(-)`)

// ExtractVersionBuildString extracts the relevant robot version from the full fw-version string
// returns normalised version string, and version.build_num for das
//
// dev version: <major>.<minor>.<build_number>
// e.g. from "v0.14.1645-4c6fb58_os0.14.1645d-42e5a97-201809031835",
// returns "00.14.00", "0.14.0.1645"
//
// prod version: <major>.<minor>.<patch>.<build_number>
// e.g. "v1.0.0.1727-96196a9_os1.0.0.1727d-73ca2a8-201809240101",
// returns "01.00.00", "1.0.0.1727"
//
func ExtractVersionBuildString(fwVersion string) (string, string) {
	if fwVersion == "" {
		alog.Warn{
			"action": "extract_version",
			"status": alog.StatusFail,
			"msg":    "empty fw string, returning default",
		}.Log()
		return DefaultFWString, DefaultDasRobotVersionString
	}

	isDevVer := true
	r := devVerRegex.FindStringSubmatch(fwVersion)
	if len(r) == 0 {
		r = prodVerRegex.FindStringSubmatch(fwVersion)
		if len(r) == 0 {
			alog.Warn{
				"action": "extract_version",
				"status": alog.StatusFail,
				"msg":    "no version string found, returning default",
				"fw":     fwVersion,
			}.Log()
			return DefaultFWString, DefaultDasRobotVersionString
		}
		isDevVer = false
	}

	ver := r[2]
	buildNumber := r[3]

	alog.Debug{
		"action":       "extract_version",
		"original_fw":  fwVersion,
		"regex_result": r,
		"version":      ver,
		"build_num":    buildNumber,
	}.Log()

	// normalize
	normalized, err := NormalizeFwVersion(ver)
	if err != nil {
		alog.Warn{
			"action":     "extract_version",
			"status":     alog.StatusFail,
			"error":      err,
			"fw_version": ver,
			"msg":        "fail to normalize version string, returning default",
		}.Log()
		normalized = DefaultFWString
	}

	dasRobotVersion := getDasRobotVersion(ver, buildNumber, isDevVer)

	return normalized, dasRobotVersion
}
