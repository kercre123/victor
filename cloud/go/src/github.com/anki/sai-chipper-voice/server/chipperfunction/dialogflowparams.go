package chipperfunction

import (
	"fmt"
	"regexp"
	"strconv"
	"strings"

	google_protobuf4 "github.com/golang/protobuf/ptypes/struct"
)

func DialogflowSingleEntity(parameters *google_protobuf4.Struct, entityName string) ChipperFunctionResult {
	return ChipperFunctionResult{
		Parameters: map[string]string{entityName: parameters.Fields[entityName].GetStringValue()},
	}
}

func DialogflowVolumeLevel(parameters *google_protobuf4.Struct) ChipperFunctionResult {
	if val, ok := parameters.Fields["entity_volume_levels"]; ok {
		return ChipperFunctionResult{
			Parameters: map[string]string{"volume_level": val.GetStringValue()},
		}
	}
	return ChipperFunctionResult{}
}

func DialogflowTimer(parameters *google_protobuf4.Struct) ChipperFunctionResult {
	durations := parameters.Fields["timer_duration"].GetListValue()
	duration := float64(0)

	for _, value := range durations.Values {
		unit := value.GetStructValue().GetFields()["unit"].GetStringValue()
		amt := value.GetStructValue().GetFields()["amount"].GetNumberValue()
		seconds, ok := TimeUnitSeconds[unit]
		if ok {
			duration += amt * seconds
		}
	}
	return ChipperFunctionResult{
		Parameters: map[string]string{
			"timer_duration": strconv.FormatInt(int64(duration), 10),
			"unit":           "s",
		},
	}
}

func DialogflowUsername(parameters *google_protobuf4.Struct, returnField, text string) ChipperFunctionResult {
	// name can either be in "word" or "given-name" field
	usernames := []string{}
	username := ""

	names := parameters.Fields["given_name"].GetListValue()
	if len(names.GetValues()) > 0 {
		for _, name := range names.Values {
			usernames = append(usernames, strings.TrimRight(name.GetStringValue(), " "))
		}
		username = strings.Join(usernames, " ")

	} else {
		// nothing in "given_name", let's look at "word"
		// "word" should always contain something since it's a @sys.any entity
		username = parameters.Fields["word"].GetStringValue()
	}

	correctName := correctUsernameCase(text, username)

	return ChipperFunctionResult{
		Parameters: map[string]string{returnField: correctName},
	}
}

func correctUsernameCase(query, username string) string {
	re := regexp.MustCompile(fmt.Sprintf("(?i)%s", username))
	found := re.FindString(query)
	if found == "" {
		return username
	} else {
		return found
	}
}

func DialogflowBehavior(parameters *google_protobuf4.Struct) ChipperFunctionResult {
	return ChipperFunctionResult{
		Parameters: map[string]string{
			"entity_behavior": parameters.Fields["entity_behavior"].GetStringValue(),
		},
	}
}

func DialogflowDeletable(parameters *google_protobuf4.Struct, text string) ChipperFunctionResult {
	params := DialogflowSingleEntity(parameters, "entity_behavior_deletable").Parameters
	name := DialogflowUsername(parameters, "username", text).Parameters
	params["given_name"] = name["username"]

	return ChipperFunctionResult{
		Parameters: params,
	}
}
