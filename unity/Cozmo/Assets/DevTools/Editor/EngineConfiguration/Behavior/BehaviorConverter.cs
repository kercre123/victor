using System;
using Newtonsoft.Json;
using Cozmo.Util;
using Newtonsoft.Json.Linq;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;

namespace Anki.Cozmo {
  internal class BehaviorConverter : JsonConverter
  {
    #region Overrides of CustomCreationConverter<Behavior>

    #region implemented abstract members of JsonConverter

    public override bool CanConvert(Type objectType) {
      return objectType == typeof(Behavior);
    }

    #endregion

    public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
    {
      writer.WriteStartObject();

      // Write properties.
      var fieldInfos = value.GetType().GetFields();
      foreach (var fieldInfo in fieldInfos)
      {
        // Skip the CustomParams field.
        if (fieldInfo.Name == "CustomParams")
          continue;

        var jsonProperty = (JsonPropertyAttribute)fieldInfo.GetCustomAttributes(typeof(JsonPropertyAttribute), true).FirstOrDefault();

        string name = fieldInfo.Name;
        if (jsonProperty != null) {
          name = jsonProperty.PropertyName;
        }

        writer.WritePropertyName(name);
        var fieldValue = fieldInfo.GetValue(value);
        serializer.Serialize(writer, fieldValue);
      }

      // Write dictionary key-value pairs.
      var behavior = (Behavior)value;
      foreach (var kvp in behavior.CustomParams)
      {
        writer.WritePropertyName(kvp.Key);
        serializer.Serialize(writer, kvp.Value);
      }
      writer.WriteEndObject();
    }

    public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
    {
      JObject jsonObject = JObject.Load(reader);
      var jsonProperties = jsonObject.Properties().ToList();
      var outputObject = (Behavior)(existingValue ?? Activator.CreateInstance(objectType));
      outputObject.CustomParams.Clear();

      // field name => field info dictionary (for fast lookup).
      var fieldNames = new HashSet<string>(objectType.GetFields().Select(
        pi => {
          var jsonProp = (JsonPropertyAttribute)pi.GetCustomAttributes(typeof(JsonPropertyAttribute), true).FirstOrDefault();
          return jsonProp != null ? jsonProp.PropertyName : pi.Name; 
        }));

      serializer.Populate(jsonObject.CreateReader(), outputObject);

      foreach (var jsonProperty in jsonProperties)
      {
        if (fieldNames.Contains(jsonProperty.Name))
        {
          continue;
        }
        else
        {
          // Otherwise - use the dictionary.
          outputObject.CustomParams.Add(jsonProperty.Name, jsonProperty.Value.ToObject<object>());
        }
      }

      return outputObject;
    }

    public override bool CanWrite
    {
      get { return true; }
    }

    #endregion
  }
}