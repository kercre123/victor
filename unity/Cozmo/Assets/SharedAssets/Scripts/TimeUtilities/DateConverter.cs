using System;
using Newtonsoft.Json;
using DataPersistence;

// Serialize Date in the format YYYYMMDD
public class DateConverter : Newtonsoft.Json.JsonConverter {
  public override bool CanConvert(Type objectType) {
    return objectType == typeof(Date);
  }

  public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer) {
    if (reader.Value == null) return null;
    if (reader.TokenType == JsonToken.String) {
      return (Date)DateTime.Parse(reader.Value.ToString());
    }
    if (reader.TokenType == JsonToken.Integer) {
      long val = ((long)reader.Value);
      return new Date((int)(val / 10000), (int)((val % 10000) / 100), (int)(val % 100));
    }

    throw new JsonReaderException("Error Reading Date: Encountered token " + reader.TokenType);
  }

  public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) {
    Date date = (Date)value;
    writer.WriteValue((long)date.Year * 10000 + (long)date.Month * 100 + (long)date.Day);
  }
}