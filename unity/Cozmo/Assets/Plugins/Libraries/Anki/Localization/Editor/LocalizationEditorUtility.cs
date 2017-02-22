using UnityEngine;
using System.Collections.Generic;

//"smartling": {
//  "translate_paths": [
//    "*/translation"
//  ],
//  "variants_enabled": true,
//  "translate_mode": "custom",
//  "placeholder_format_custom": [
//    "%%[^%]+?%%",
//    "%[^%]+?%",
//    "##[^#]+?##",
//    "__[^_]+?__",
//    "[$]\\{[^\\}\\{]+?\\}",
//    "\\{\\{[^\\}\\{]+?\\}\\}",
//    "(?<!\\{)\\{[^\\}\\{]+?\\}(?!\\})"
//  ],
//  "source_key_paths": [
//    "/{*}"
//  ]
//},
using Newtonsoft.Json;
using System.ComponentModel;
using System.Collections;
using System.IO;
using UnityEditor;
using Newtonsoft.Json.Converters;
using System;
using System.Linq;
using System.Reflection;
using Newtonsoft.Json.Linq;

public class SmartlingBlob {
  [JsonProperty("translate_paths")]
  public List<string> TranslatePaths = new List<string>();

  [JsonProperty("variants_enabled")]
  public bool VariantsEnabled;

  [JsonProperty("translate_mode")]
  public string TranslateMode;

  [JsonProperty("placeholder_format_custom")]
  public List<string> PlaceholderFormatCustom = new List<string>();

  [JsonProperty("source_key_paths")]
  public List<string> SourceKeyPaths = new List<string>();
}


[JsonConverter(typeof(LocalizationDictionaryConverter))]
public class LocalizationDictionary {
  public Dictionary<string, LocalizationDictionaryEntry> Translations { get; set; }

  [JsonProperty("smartling")]
  public SmartlingBlob Smartling { get; set; }

}

public class LocalizationDictionaryEntry {
  [JsonProperty("translation")]
  public string Translation;
}

internal class LocalizationDictionaryConverter : CustomCreationConverter<LocalizationDictionary> {
  #region Overrides of CustomCreationConverter<LocalizationDictionary>

  public override LocalizationDictionary Create(Type objectType) {
    return new LocalizationDictionary {
      Translations = new Dictionary<string, LocalizationDictionaryEntry>()
    };
  }

  public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) {
    writer.WriteStartObject();
    var dict = (LocalizationDictionary)value;

    // Write properties.
    writer.WritePropertyName("smartling");
    serializer.Serialize(writer, dict.Smartling);

    // Write dictionary key-value pairs.
    foreach (var kvp in dict.Translations) {
      writer.WritePropertyName(kvp.Key);
      serializer.Serialize(writer, kvp.Value);
    }
    writer.WriteEndObject();
  }

  public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer) {
    JObject jsonObject = JObject.Load(reader);
    var jsonProperties = jsonObject.Properties().ToList();
    var outputObject = Create(objectType);

    foreach (var jsonProperty in jsonProperties) {
      // If such property exists - use it.
      if (jsonProperty.Name == "smartling") {
        var propertyValue = jsonProperty.Value.ToObject<SmartlingBlob>();
        outputObject.Smartling = propertyValue;
      }
      else {
        // Otherwise - use the dictionary.
        outputObject.Translations.Add(jsonProperty.Name, jsonProperty.Value.ToObject<LocalizationDictionaryEntry>());
      }
    }

    return outputObject;
  }

  public override bool CanWrite {
    get { return true; }
  }

  #endregion
}

public static class LocalizationEditorUtility {

  private static readonly Dictionary<string, LocalizationDictionary> _LocalizationDictionaries = new Dictionary<string, LocalizationDictionary>();
  private static string[] _LocalizationFiles = new string[0];

  private static string[] _LocalizationKeys = new string[0];

  private const string kLocalizationFolder = "Assets/StreamingAssets/LocalizedStrings/en-US/";

  public static string[] LocalizationFiles { get { return _LocalizationFiles; } }

  public static string[] LocalizationKeys { get { return _LocalizationKeys; } }

  public static readonly char[] kFormatMarkers = { '<', '>' };

  public static LocalizationDictionary CreateLocalizationDictionary() {
    return new LocalizationDictionary() {
      Smartling = new SmartlingBlob() {
        TranslatePaths = new List<string> { "*/translation" },
        VariantsEnabled = true,
        TranslateMode = "custom",
        PlaceholderFormatCustom = new List<string> {
          "%%[^%]+?%%",
          "%[^%]+?%",
          "##[^#]+?##",
          "__[^_]+?__",
          "[$]\\{[^\\}\\{]+?\\}",
          "\\{\\{[^\\}\\{]+?\\}\\}",
          "(?<!\\{)\\{[^\\}\\{]+?\\}(?!\\})"
        },
        SourceKeyPaths = new List<string> { "/{*}" }
      },
      Translations = new Dictionary<string, LocalizationDictionaryEntry>()
    };
  }

  public static LocalizationDictionary CreateNewLocalizationFile(string fileName) {
    var dict = CreateLocalizationDictionary();

    File.WriteAllText(kLocalizationFolder + fileName + ".json", JsonConvert.SerializeObject(dict, Formatting.Indented));

    _LocalizationDictionaries[fileName] = dict;
    return dict;
  }

  static LocalizationEditorUtility() {
    Reload();
  }


  [MenuItem("Cozmo/Localization/Reload Localization Files")]
  public static void Reload() {
    _LocalizationDictionaries.Clear();

    foreach (var file in Directory.GetFiles(kLocalizationFolder, "*.json")) {
      _LocalizationDictionaries[Path.GetFileNameWithoutExtension(file)] = JsonConvert.DeserializeObject<LocalizationDictionary>(File.ReadAllText(file));
    }

    _LocalizationFiles = _LocalizationDictionaries.Keys.ToArray();

    _LocalizationKeys = new[] { string.Empty }.Concat(_LocalizationDictionaries.Values.SelectMany(x => x.Translations.Keys)).ToArray();
    // sort them to make it easier to find
    Array.Sort(_LocalizationKeys);
  }

  public static string GetTranslationSansFormatting(string key) {
    string fileName = "";
    string sansFormat = GetTranslation(key, out fileName);
    string[] splitName = sansFormat.Split();
    for (int i = 0; i < splitName.Length; i++) {
      int markerStart = splitName[i].IndexOf(kFormatMarkers[0]);
      int markerEnd = splitName[i].IndexOf(kFormatMarkers[1]);
      while (markerStart >= 0 && markerEnd > markerStart) {
        splitName[i] = splitName[i].Remove(markerStart, markerEnd - markerStart + 1);
        markerStart = splitName[i].IndexOf(kFormatMarkers[0]);
        markerEnd = splitName[i].IndexOf(kFormatMarkers[1]);
      }
    }
    sansFormat = String.Join(" ", splitName);
    return sansFormat;
  }

  public static string GetTranslation(string fileName, string key) {
    LocalizationDictionary dict;
    if (_LocalizationDictionaries.TryGetValue(fileName, out dict)) {
      LocalizationDictionaryEntry entry;
      if (dict.Translations.TryGetValue(key, out entry)) {
        return entry.Translation;
      }
    }
    return string.Empty;
  }

  // find key in any file
  public static string GetTranslation(string key, out string fileName) {
    foreach (var kvp in _LocalizationDictionaries) {
      var dict = kvp.Value;
      fileName = kvp.Key;
      LocalizationDictionaryEntry entry;
      if (dict.Translations.TryGetValue(key, out entry)) {
        return entry.Translation;
      }
    }
    fileName = string.Empty;
    return string.Empty;
  }

  public static bool KeyExists(string fileName, string key) {
    LocalizationDictionary dict;
    if (_LocalizationDictionaries.TryGetValue(fileName, out dict)) {
      return dict.Translations.ContainsKey(key);
    }
    return false;
  }

  public static void SetTranslation(string fileName, string key, string translation) {
    LocalizationDictionary dict;
    if (!_LocalizationDictionaries.TryGetValue(fileName, out dict)) {
      dict = CreateLocalizationDictionary();
      _LocalizationDictionaries[fileName] = dict;
    }

    LocalizationDictionaryEntry entry;
    if (!dict.Translations.TryGetValue(key, out entry)) {
      entry = new LocalizationDictionaryEntry();
      dict.Translations[key] = entry;
    }

    entry.Translation = translation;

    File.WriteAllText(kLocalizationFolder + fileName + ".json", JsonConvert.SerializeObject(dict, Formatting.Indented));
  }

  private const string kGeneratedLocalizationKeysFilePath = "Assets/Plugins/Libraries/Anki/Localization/GeneratedKeys/LocalizationKeys.cs";
  private const string kGeneratedLocalizationKeysSourceLocale = "en-us";

  [MenuItem("Cozmo/Localization/Generate Localization Key Constants")]
  private static void GenerateLocalizationKeyConstFile() {
    string fileContents = "public static class LocalizationKeys {";

    // For every key in localization, generate a C# constant. Use english as the source.
    JSONObject localizationJson;
    string cSharpVariableName;
    foreach (var jsonFileName in Localization.GetLocalizationJsonFilePaths(kGeneratedLocalizationKeysSourceLocale)) {

      fileContents += "\n\n  #region " + Path.GetFileNameWithoutExtension(jsonFileName) + "\n";

      localizationJson = Localization.GetJsonContentsFromLocalizationFile(kGeneratedLocalizationKeysSourceLocale, jsonFileName);
      foreach (string localizationKey in localizationJson.keys) {
        // Skip smartling data
        if ("smartling" == localizationKey) {
          continue;
        }

        // Create a const variable using the key that follows convention
        cSharpVariableName = VariableNameFromLocalizationKey(localizationKey);
        fileContents += "\n  public const string " + cSharpVariableName + " = \"" + localizationKey + "\";";
      }

      fileContents += "\n\n  #endregion";
    }

    fileContents += "\n}";

    // Write the code to a file
    File.WriteAllText(kGeneratedLocalizationKeysFilePath, fileContents);
  }

  [MenuItem("Cozmo/Localization/Copy Boot String Files")]
  public static void CopyBootStringFiles() {
    // Bootup strings need to be in resources, so Copy them in the event something has changed.
    // TextAssets don't get called as a part of the AssetPostProcessor. So just manually copy them.
    const string _kBootStringsFileName = "/BootStrings.json";
    string[] supportedLocaleDirs = Localization.GetLocalizationAllLanguagesPaths();
    foreach (string srcPath in supportedLocaleDirs) {
      if (File.Exists(srcPath + _kBootStringsFileName)) {
        string[] splitStr = srcPath.Split('/');
        string combinedPath = Path.Combine(srcPath, "../../../Resources/bootstrap/LocalizedStrings/");
        string destPath = combinedPath + splitStr[splitStr.Length - 1] + _kBootStringsFileName;
        File.Copy(srcPath + _kBootStringsFileName, destPath, true);
      }
    }
  }

  private static string VariableNameFromLocalizationKey(string localizationKey) {
    string variableName = "k";
    char[] delimiter = { '.', ' ' };
    string[] keyParts = localizationKey.Split(delimiter, StringSplitOptions.RemoveEmptyEntries);
    foreach (string part in keyParts) {
      variableName += char.ToUpper(part[0]) + part.Substring(1);
    }
    return variableName;
  }

  private static bool StringContainedInDir(string str, string dir, string searchPattern, string exclude = null) {
    foreach (var path in Directory.GetFiles(dir, searchPattern, SearchOption.AllDirectories)) {
      if (exclude != null && path.Contains(exclude)) {
        continue;
      }
      if (File.ReadAllText(path).Contains(str)) {
        return true;
      }
    }
    return false;
  }
  // Can't search with multiple file extensions
  private static bool StringContainedInDir(string str, string dir, string[] searchPatterns) {
    foreach (var path in Directory.GetFiles(dir, "*", SearchOption.AllDirectories)) {
      for (int i = 0; i < searchPatterns.Length; ++i) {
        if (path.Contains(searchPatterns[i])) {
          if (File.ReadAllText(path).Contains(str)) {
            return true;
          }
          break;
        }
      }
    }
    return false;
  }

  [MenuItem("Cozmo/Localization/Remove Unused Loc Keys")]
  public static void RemoveUnusedLocKeys() {
    foreach (var localizationDictFile in _LocalizationDictionaries) {
      string fileName = localizationDictFile.Key;
      // Special case file that adds plural etc
      if (fileName == "ItemStrings") {
        continue;
      }
      // Deep copy to remove from source
      Dictionary<string, LocalizationDictionaryEntry> keysDict = new Dictionary<string, LocalizationDictionaryEntry>(localizationDictFile.Value.Translations);
      bool anyUpdateNeeded = false;
      foreach (var locKey in keysDict) {
        bool keyFound = false;
        // Check C# code, prefabs, assets.
        // Check configs for game
        // Check Config code from engine...
        if (locKey.Key.Contains(".plural") || locKey.Key.Contains(".singular") ||
            StringContainedInDir(VariableNameFromLocalizationKey(locKey.Key), "Assets/", "*.cs", "LocalizationKeys.cs") ||
            StringContainedInDir(locKey.Key, "Assets/", new string[] { ".prefab", ".asset" }) ||
            StringContainedInDir(locKey.Key, Application.dataPath + "/../../../lib/anki/products-cozmo-assets/", "*.json") ||
            StringContainedInDir(locKey.Key, Application.dataPath + "/../../../resources/config/basestation/config", "*.json")
            ) {
          // orring for fast out
          keyFound = true;
        }

        // Consider a key not used if it is not in C# code, prefabs, or engine configs
        if (!keyFound) {
          localizationDictFile.Value.Translations.Remove(locKey.Key);
          anyUpdateNeeded = true;
          Debug.LogWarning("Not Found " + locKey.Key);
        }
      }

      if (anyUpdateNeeded) {
        File.WriteAllText(kLocalizationFolder + fileName + ".json", JsonConvert.SerializeObject(localizationDictFile.Value, Formatting.Indented));
      }
    }
  }

  private static List<string> AddFilesContainingStringInDir(ref List<string> filesContainingStr, string str, string dir, string searchPattern, string exclude = null) {
    foreach (var path in Directory.GetFiles(dir, searchPattern, SearchOption.AllDirectories)) {
      if (exclude != null && path.Contains(exclude)) {
        continue;
      }
      if (File.ReadAllText(path).Contains(str)) {
        string[] splitPath = path.Split(new char[] { '/' });
        filesContainingStr.Add(splitPath[splitPath.Length - 1]);
      }
    }
    return filesContainingStr;
  }
  // Can't search with multiple file extensions
  private static List<string> AddFilesContainingStringInDir(ref List<string> filesContainingStr, string str, string dir, string[] searchPatterns) {
    foreach (var path in Directory.GetFiles(dir, "*", SearchOption.AllDirectories)) {
      for (int i = 0; i < searchPatterns.Length; ++i) {
        if (path.Contains(searchPatterns[i])) {
          if (File.ReadAllText(path).Contains(str)) {
            string[] splitPath = path.Split(new char[] { '/' });
            filesContainingStr.Add(splitPath[splitPath.Length - 1]);
          }
          break; // move on to next path
        }
      }
    }
    return filesContainingStr;
  }

  private const string kLocKeyReferenceReportFilePath = "LocKeyReferenceReport.csv";

  [MenuItem("Cozmo/Localization/Report Loc Key References")]
  private static void ReportLocKeyReferences() {
    List<LocKeyReference> locKeyReferences = new List<LocKeyReference>();

    foreach (var localizationDictFile in _LocalizationDictionaries) {
      string fileName = localizationDictFile.Key;
      // Special case file that adds plural etc
      if (fileName == "ItemStrings") {
        continue;
      }

      // Walk through keys to find incidence
      Dictionary<string, LocalizationDictionaryEntry> keysDict = localizationDictFile.Value.Translations;
      foreach (var locKey in keysDict) {
        List<string> pathsContainingKey = new List<string>();
        AddFilesContainingStringInDir(ref pathsContainingKey, VariableNameFromLocalizationKey(locKey.Key), "Assets/", "*.cs", "LocalizationKeys.cs");
        AddFilesContainingStringInDir(ref pathsContainingKey, locKey.Key, "Assets/", new string[] { ".prefab", ".asset" });
        AddFilesContainingStringInDir(ref pathsContainingKey, locKey.Key, Application.dataPath + "/../../../lib/anki/products-cozmo-assets/", "*.json");
        AddFilesContainingStringInDir(ref pathsContainingKey, locKey.Key, Application.dataPath + "/../../../resources/config/basestation/config", "*.json");

        LocKeyReference reference = new LocKeyReference();
        reference.Key = locKey.Key;
        reference.Value = locKey.Value.Translation;
        reference.FilesThatReference = pathsContainingKey;
        locKeyReferences.Add(reference);
      }
    }

    locKeyReferences.Sort((x, y) => {
      return x.FilesThatReference.Count.CompareTo(y.FilesThatReference.Count);
    });

    string fileContents = "Loc Value,Loc Key,Files\n";
    foreach (var reference in locKeyReferences) {
      fileContents += "\"" + reference.Value.Replace("\n", " ").Replace("\"", "") + "\"," + reference.Key + ",";
      fileContents += reference.FilesThatReference[0] + "\n";
      for (int i = 1; i < reference.FilesThatReference.Count; i++) {
        fileContents += ",," + reference.FilesThatReference[i] + "\n";
      }
    }

    // Write the to a file
    File.WriteAllText(kLocKeyReferenceReportFilePath, fileContents);
    Debug.Log("Report written to " + kLocKeyReferenceReportFilePath);
  }

  private struct LocKeyReference {
    public string Key;
    public string Value;
    public List<string> FilesThatReference;
  }
}
