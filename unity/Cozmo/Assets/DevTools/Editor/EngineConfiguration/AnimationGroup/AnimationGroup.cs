using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Linq;

namespace AnimationGroups {
  public class AnimationGroup {

    public List<AnimationGroupEntry> Animations = new List<AnimationGroupEntry>();

    [JsonConverter(typeof(AnimationGroupConverter))]
    [System.Serializable]
    public class AnimationGroupEntry {

      public string Name;

      public float Weight = 1;

      public float CooldownTime_Sec;

      public SimpleMoodType Mood;

      // OPTIONAL Parameters, checked in custom serielizer
      public bool UseHeadAngle = false;
      public float HeadAngleMin_Deg = -25.0f;
      public float HeadAngleMax_Deg = 45.0f;
    }
  }

  public class AnimationGroupConverter : Newtonsoft.Json.JsonConverter {
    public override bool CanConvert(System.Type objectType) {
      return objectType == typeof(AnimationGroup.AnimationGroupEntry);
    }

    // just do the default
    public override object ReadJson(JsonReader reader, System.Type objectType, object existingValue, JsonSerializer serializer) {
      JObject jsonObject = JObject.Load(reader);
      var outputObject = (AnimationGroup.AnimationGroupEntry)(existingValue ?? System.Activator.CreateInstance(objectType));
      serializer.Populate(jsonObject.CreateReader(), outputObject);
      return outputObject;
    }

    public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) {
      writer.WriteStartObject();

      AnimationGroup.AnimationGroupEntry entry = (AnimationGroup.AnimationGroupEntry)value;
      // Write properties.
      var fieldInfos = value.GetType().GetFields();
      foreach (var fieldInfo in fieldInfos) {
        // Skip the Optional params, we'll manaully check if
        if (fieldInfo.Name == "HeadAngleMin_Deg" || fieldInfo.Name == "HeadAngleMax_Deg" || fieldInfo.Name == "UseHeadAngle") {
          if (!entry.UseHeadAngle)
            continue;
        }

        var jsonProperty = (JsonPropertyAttribute)fieldInfo.GetCustomAttributes(typeof(JsonPropertyAttribute), true).FirstOrDefault();

        string name = fieldInfo.Name;
        if (jsonProperty != null) {
          name = jsonProperty.PropertyName;
        }

        writer.WritePropertyName(name);
        var fieldValue = fieldInfo.GetValue(value);
        serializer.Serialize(writer, fieldValue);
      }
      writer.WriteEndObject();
    }
  }

}

