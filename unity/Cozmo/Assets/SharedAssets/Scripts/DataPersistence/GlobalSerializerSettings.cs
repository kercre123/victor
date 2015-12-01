using System;
using Newtonsoft.Json;
using System.Collections.Generic;

public static class GlobalSerializerSettings {
  private static JsonSerializerSettings _JsonSettings;

  public static JsonSerializerSettings JsonSettings {
    get {
      if (_JsonSettings == null) {
        _JsonSettings = new JsonSerializerSettings() {
          TypeNameHandling = TypeNameHandling.Auto,
          Converters = new List<JsonConverter> {
            new UtcDateTimeConverter(),
            new Newtonsoft.Json.Converters.StringEnumConverter()
          }
        };
      }
      return _JsonSettings;
    }
  }
}

