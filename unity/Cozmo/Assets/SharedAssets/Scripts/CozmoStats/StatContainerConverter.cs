using System;
using Newtonsoft.Json;
using Cozmo.Util;
using Newtonsoft.Json.Linq;
using System.Linq;
using System.Reflection;
using System.Collections.Generic;
using Anki.Cozmo;

internal class StatContainerConverter : JsonConverter
{
  #region Overrides of CustomCreationConverter<StatContainer>

  #region implemented abstract members of JsonConverter

  public override bool CanConvert(Type objectType) {
    return objectType == typeof(StatContainer);
  }

  #endregion

  public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
  {
    writer.WriteStartObject();

    var statContainer = (StatContainer)value;

    // Write non-zero stats.
    foreach (var stat in StatContainer.sKeys) {      
      if (statContainer[stat] != 0) {
        writer.WritePropertyName(stat.ToString());
        serializer.Serialize(writer, statContainer[stat]);
      }
    }
    writer.WriteEndObject();
  }

  public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
  {
    if (reader.TokenType == JsonToken.StartObject) {
      JObject jsonObject = JObject.Load(reader);
      var jsonProperties = jsonObject.Properties().ToList();
      var outputObject = (StatContainer)existingValue ?? new StatContainer();
      outputObject.Clear();

      foreach (var jsonProperty in jsonProperties) {
        try { 
          var key = (ProgressionStatType)Enum.Parse(typeof(ProgressionStatType), jsonProperty.Name);
          outputObject[key] = jsonProperty.Value.ToObject<int>();
        }
        catch (Exception ex) {
          // Just Ignore any invalid properties
          DAS.Error(this, ex);
        }
      }
      return outputObject;
    }
    else if(reader.TokenType == JsonToken.Null) {
      return null;
    }

    throw new JsonReaderException("Unexpected token " + reader.TokenType + " while reading StatContainer");
  }

  public override bool CanWrite
  {
    get { return true; }
  }

  #endregion
}
