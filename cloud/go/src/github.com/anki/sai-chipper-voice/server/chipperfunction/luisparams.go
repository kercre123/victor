package chipperfunction

import (
	ms "github.com/anki/sai-chipper-voice/conversation/microsoft"
	"github.com/anki/sai-chipper-voice/conversation/util"
)

const (
	LuisTypeTime          = "builtin.datetimeV2.time"
	LuisTypeDateTime      = "builtin.datetimeV2.datetime"
	LuisTypeDatetimeRange = "builtin.datetimeV2.datetimerange"
	LuisTypeDate          = "builtin.datetimeV2.date"
	LuisTypeDuration      = "builtin.datetimeV2.duration"
	LuisTypeGeoUSState    = "builtin.geography.us_state"
	LuisTypeGeoCity       = "builtin.geography.city"
	LuisTypeGeoCountry    = "builtin.geography.country"
	LuisTypeBehavior      = "entity_behavior"
	LuisTypeName          = "name"
	LuisTypeGivenName     = "given-name"
	LuisTypeWord          = "word"
	LuisTypeForecastWord  = "entity_forecast_words"
)

func LuisSingleEntity(entities []ms.LuisResponseEntity, entityName string) ChipperFunctionResult {
	for _, e := range entities {
		if e.Type == entityName {
			flatten, _ := util.FlattenInterfaceToStringMap("", e.Resolution.Values)
			return ChipperFunctionResult{
				Parameters: map[string]string{entityName: flatten["0"]},
			}
		}
	}
	return ChipperFunctionResult{}

}

func LuisGivenName(entities []ms.LuisResponseEntity) ChipperFunctionResult {
	for _, e := range entities {
		if e.Type == LuisTypeWord || e.Type == LuisTypeGivenName {
			// either "word" or "given-name" will be returned
			return ChipperFunctionResult{
				Parameters: map[string]string{"given_name": e.Entity},
			}
		}
	}
	return ChipperFunctionResult{}

}

func LuisUsername(entities []ms.LuisResponseEntity) ChipperFunctionResult {
	for _, e := range entities {
		if e.Entity != "" && (e.Type == LuisTypeName || e.Type == LuisTypeWord) {
			return ChipperFunctionResult{
				Parameters: map[string]string{"username": e.Entity},
			}
		}
	}
	return ChipperFunctionResult{}
}

func LuisExtractEntityValue(er *ms.LuisResponseEntityResolution, valueType, valueField string) string {
	for _, value := range *er.Values {
		flatten, _ := util.FlattenInterfaceToStringMap("", value)
		vtype, ok := flatten[".type"]
		if ok && vtype == valueType {
			return flatten["."+valueField]
		}
	}
	return ""
}

func LuisTimer(entities []ms.LuisResponseEntity) ChipperFunctionResult {
	for _, e := range entities {
		if e.Type == LuisTypeDuration && e.Resolution.Values != nil {
			duration := LuisExtractEntityValue(e.Resolution, "duration", "value")
			if duration != "" {
				return ChipperFunctionResult{
					Parameters: map[string]string{
						"timer_duration": duration,
						"unit":           "s",
					},
				}
			}
			break
		}
	}
	return ChipperFunctionResult{}
}

func LuisBehavior(entities []ms.LuisResponseEntity) ChipperFunctionResult {
	for _, e := range entities {
		if e.Type == LuisTypeBehavior && e.Resolution.Values != nil {
			for _, value := range *e.Resolution.Values {
				flatten, _ := util.FlattenInterfaceToStringMap("", value)
				return ChipperFunctionResult{
					Parameters: map[string]string{
						"entity_behavior": flatten[""],
					},
				}
			}
			break
		}
	}
	return ChipperFunctionResult{}
}
