package chipperfunction

import (
	"encoding/json"
	"regexp"
	"strconv"
	"strings"

	google_protobuf4 "github.com/golang/protobuf/ptypes/struct"

	"github.com/anki/sai-go-util/log"

	ms "github.com/anki/sai-chipper-voice/conversation/microsoft"
	"github.com/anki/sai-chipper-voice/conversation/util"
)

const (
	HourInSeconds           = float64(3600)
	MinuteInSeconds         = float64(60)
	FallbackIntent          = "intent_system_unsupported"
	IntentNoLocation        = "intent_weather_unknownlocation"
	IntentNoDefaultLocation = "intent_weather_nodefaultlocation"
)

var TimeUnitSeconds = map[string]float64{"h": HourInSeconds, "min": MinuteInSeconds, "s": float64(1)}

var LexTimerPattern = regexp.MustCompile(`^(PT)([0-9]+H)*([0-9]+M)*([0-9]+S)*`)

type ChipperFunctionResult struct {
	Parameters map[string]string
	NewIntent  string
}

//
// Dialogflow
//

func ParseDialogflow(intentName, text string, parameters *google_protobuf4.Struct) ChipperFunctionResult {
	// Debug -- REMOVE before deploy
	//fmt.Println(parameters)
	//ejson, _ := json.MarshalIndent(parameters, "", "    ")
	//fmt.Println(string(ejson))

	switch strings.ToLower(intentName) {
	case "intent_clock_settimer", "intent_clock_settimer_extend":
		return DialogflowTimer(parameters)

	case "intent_names_username", "intent_names_username_extend":
		return DialogflowUsername(parameters, "username", text)

	case "intent_weather", "intent_weather_extend":
		return ChipperFunctionResult{
			NewIntent:  IntentNoLocation,
			Parameters: nil,
		}
	case "intent_play_specific", "intent_play_specific_extend":
		return DialogflowBehavior(parameters)

	case "intent_message_playmessage", "intent_message_playmessage_extend",
		"intent_message_recordmessage", "intent_message_recordmessage_extend":
		return DialogflowUsername(parameters, "given_name", text)

	case "intent_global_stop", "intent_global_stop_extend":
		return DialogflowSingleEntity(parameters, "entity_behavior_stoppable")

	case "intent_global_delete", "intent_global_delete_extend":
		return DialogflowDeletable(parameters, text)

	case "intent_photo_take", "intent_photo_take_extend":
		return DialogflowSingleEntity(parameters, "entity_photo_selfie")

	case "intent_system_discovery", "intent_system_discovery_extend":
		return DialogflowSingleEntity(parameters, "entity_topic")

	case "intent_imperative_volumelevel_extend":
		return DialogflowVolumeLevel(parameters)
	}

	return ChipperFunctionResult{}
}

//
// Luis
//

func ParseLuis(intentName, text string, luisResponseBlob []byte) ChipperFunctionResult {

	// Transform luisResponseBlob to json
	var luisResponse ms.LuisResponse
	if err := json.Unmarshal(luisResponseBlob, &luisResponse); err != nil {
		return ChipperFunctionResult{}
	}

	// For Debug -- REMOVE BEFORE DEPLOY
	//ejson, _ := json.MarshalIndent(luisResponse.Entities, "", "    ")
	//fmt.Println("LUIS PARAMTERS:\n", string(ejson))

	entities := luisResponse.Entities
	if len(entities) == 0 && intentName != "intent_weather" {
		alog.Debug{"action": "luis_no_parameters"}.Log()
		return ChipperFunctionResult{}
	}

	switch strings.ToLower(intentName) {
	case "intent_clock_settimer", "intent_clock_settimer_extend":
		return LuisTimer(entities)

	case "intent_names_username", "intent_names_username_extend",
		"intent_names_ask":
		// Luis keeps matching "My name is [name that is not Tom]" to intent_names_ask
		return LuisUsername(entities)

	case "intent_weather", "intent_weather_extend":
		return ChipperFunctionResult{
			NewIntent:  IntentNoLocation,
			Parameters: nil,
		}

	case "intent_play_specific", "intent_play_specific_extend":
		return LuisBehavior(entities)

	case "intent_message_playmessage", "intent_message_playmessage_extend",
		"intent_message_recordmessage", "intent_message_recordmessage_extend":
		return LuisGivenName(entities)

	// TODO: Luis is confusing a lot of the entities below
	// because they have the same string for different entity names.
	// Example: "timer" is in entity_behavior_stoppable and entity_behavior_deletable and entity_topic
	case "intent_global_stop", "intent_global_stop_extend":
		return LuisSingleEntity(entities, "entity_behavior_stoppable")

	case "intent_global_delete", "intent_global_delete_extend",
		"intent_message_delete":
		// TODO: remove "intent_message_delete" after typo corrected
		return LuisSingleEntity(entities, "entity_behavior_deletable")

	case "intent_photo_take", "intent_photo_take_extend":
		return LuisSingleEntity(entities, "entity_photo_selfie")

	case "intent_system_discovery", "intent_system_discovery_extend":
		return LuisSingleEntity(entities, "entity_topic")

	}

	return ChipperFunctionResult{}
}

//
// Lex
//

func ParseLexEntities(intentName string, input interface{}) map[string]string {
	alog.Debug{"Lex_original": input}.Log()

	// Debug -- REMOVE before deploy
	//ejson, _ := json.MarshalIndent(input, "", "    ")
	//fmt.Println(string(ejson))

	if intentName == FallbackIntent {
		return map[string]string{}
	}

	entitiesMap, err := util.FlattenInterfaceToStringMap("", input)
	if err != nil {
		alog.Warn{"action": "parse_lex_text_entities", "error": err}.Log()
	}

	alog.Debug{"action": "parse_lex_params", "flatten": entitiesMap}.Log()

	results := make(map[string]string)

	switch strings.ToLower(intentName) {
	case "intent_clock_settimer", "intent_clock_settimer_extend":
		results["timer_duration"] = getLexDuration(entitiesMap)
		results["unit"] = "s"

	case "intent_names_username", "intent_names_username_extend":
		results["username"] = getLexNames(entitiesMap)

	case "intent_message_playmessage_extend", "intent_message_recordmessage_extend":
		results["given_name"] = getLexNames(entitiesMap)

	case "intent_weather", "intent_weather_extend":
		return nil

	case "intent_global_delete", "intent_global_delete_extend":
		results["given_name"] = getLexNames(entitiesMap)
		results["entity_behavior_deletable"] = getLexSingleEntity(entitiesMap, ".entity_behavior_deletable")

	case "intent_global_stop", "intent_global_stop_extend":
		results["entity_behavior_stoppable"] = getLexSingleEntity(entitiesMap, ".entity_behavior_stoppable")

	case "intent_photo_take", "intent_photo_take_extend":
		results["entity_photo_selfie"] = getLexSingleEntity(entitiesMap, ".entity_photo_selfie")

	case "intent_imperative_volumelevel_extend":
		results["volume_level"] = getLexSingleEntity(entitiesMap, ".entity_volume_levels")

	default:
		for k, v := range entitiesMap {
			newKey := k[1:]
			results[newKey] = v

		}
	}

	return results
}

func getLexSingleEntity(values map[string]string, entityName string) string {
	if val, ok := values[entityName]; ok {
		return val
	}
	return ""
}

func getLexNames(values map[string]string) string {
	if value, ok := values[".given_name"]; ok {
		return value
	} else if value, ok := values[".word"]; ok {
		return value
	} else {
		return ""
	}
}

func getLexDuration(values map[string]string) string {
	duration := int64(0)
	for _, v := range values {
		if v != "" {
			duration += parseLexDuration(v)
		}
	}
	return strconv.FormatInt(duration, 10)
}

func parseLexDuration(text string) int64 {
	m := LexTimerPattern.FindStringSubmatch(text)
	duration := int64(0)
	for _, v := range m {
		if strings.Contains(v, "PT") || v == "" {
			continue
		}

		num, err := strconv.ParseInt(v[:len(v)-1], 10, 64)
		if err == nil {
			if strings.Contains(v, "M") {
				num = num * int64(MinuteInSeconds)

			} else if strings.Contains(v, "H") {
				num = num * int64(HourInSeconds)
			}
			duration += num
		}
	}
	return duration
}
