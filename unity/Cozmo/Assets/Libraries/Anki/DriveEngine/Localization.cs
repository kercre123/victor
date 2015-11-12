using UnityEngine;
using System.Collections.Generic;
using System;

public static class Localization {

  private static Anki.AppResources.StringTable _st = new Anki.AppResources.StringTable();

  public static bool showDebugLocText = false;

  public static string Get(string key) {
    string value;
    if (!_st.TryGetString(key, out value)) {
      #if !UNITY_EDITOR
      DAS.Warn("LocalizedString.Get.MissingKey", "");  //disabling populating of key name for COPPA reasons (JDN)
      #else
      DAS.Warn("LocalizedString.Get.MissingKey", key);
      #endif
      value = key;
    }
    else if (showDebugLocText) {
      value = new string('@', value.Length);
    }

    return value;
  }

  public static bool IsSupportedLocale(string locale) {
    return _SupportedLocales.Contains(locale);
  }

  private static List<string> _SupportedLocales = 
    new List<string>() { "en-US", "en-GB", "de-DE" };

  // Return locale code for a language code.
  public static string GetStringsLocale() {

    // TODO: INGO - Need to provide functionality to GetCurrentLocale?
    // string lang;
    // string country;
    // string deviceLocale = Anki.ApplicationServices.GetCurrentLocale(out lang, out country);
    string lang = "en";
    string deviceLocale = "en-US";

    if (_SupportedLocales.Contains(deviceLocale)) {
      // TODO: Biggest hack ever, but we don't currently have any text data for en-GB, so just hard code it back to en-US
      if (deviceLocale == "en-GB") {
        deviceLocale = "en-US";
      }

      return deviceLocale;
    }

    string locale;
    switch (lang) {
    case "en":
      locale = "en-US";
      break;
    case "de":
      locale = "de-DE";
      break;
    default:
      locale = "en-US";
      break;
    }

    return locale;
  }

  public static void LoadStrings() {
    // TODO(BRC) Dynamically read variable number of strings files
    string[] resources = {
      "overdrive-strings.json",
      "asset-strings.json",
      "overlay-strings-custom.json"
    };

    string locale = GetStringsLocale();
    for (int i = 0; i < resources.Length; ++i) {
      string resource = "localized-strings/" + locale + "/" + resources[i];

      // TODO: INGO - need to analyze what GetPathForResource does?
      //string path = Anki.DriveEngine.Platform.GetPathForResource(resource);
      string path = resource;

      Anki.AppResources.StringTable st = Anki.AppResources.StringTable.LoadStringsFromSmartlingJSONFile(path);
      _st.MergeEntriesFromStringTable(st);
    }
  }
}