using System;
using Newtonsoft.Json;

public class UtcDateTimeConverter : Newtonsoft.Json.JsonConverter {
  public override bool CanConvert(Type objectType) {
    return objectType == typeof(DateTime);
  }

  public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer) {
    if (reader.Value == null) return null;
    if (reader.TokenType == JsonToken.String) {
      return DateTime.Parse(reader.Value.ToString());
    }
    if (reader.TokenType == JsonToken.Integer) {
      return ((long)reader.Value).FromUtcMs();
    }

    throw new JsonReaderException("Error Reading DateTime: Encountered token " + reader.TokenType);
  }

  public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) {
    writer.WriteValue(((DateTime) value).ToUniversalTime().ToUtcMs());
  }
}