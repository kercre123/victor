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
      if (Application.isPlaying) {
        DAS.Warn("LocalizedString.Get.MissingKey", key);
      }
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

  private static string _CurrentLocale = null;
  private static System.Globalization.CultureInfo _CurrentCulture = null;

  // Return locale code for a language code.
  public static string GetStringsLocale() {

    if (string.IsNullOrEmpty(_CurrentLocale)) {
      // TODO: INGO - Need to provide functionality for GetCurrentLocale that 
      // doesn't rely on DriveEngine
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
      }
      else {
        switch (lang) {
        case "en":
          deviceLocale = "en-US";
          break;
        case "de":
          deviceLocale = "de-DE";
          break;
        default:
          deviceLocale = "en-US";
          break;
        }
      }
        
      _CurrentLocale = deviceLocale;
      _CurrentCulture = new System.Globalization.CultureInfo(_CurrentLocale);
    }

    return _CurrentLocale;
  }

  public static void LoadStrings() {
    // TODO(BRC) Dynamically read variable number of strings files
    string[] resources = {
      "HubWorldStrings",
      "MinigameStrings"
    };

    string locale = GetStringsLocale();
    for (int i = 0; i < resources.Length; ++i) {
      string resourceFilePath = "LocalizedStrings/" + locale + "/" + resources[i];

      TextAsset languageAsset = Resources.Load(resourceFilePath, typeof(TextAsset)) as TextAsset;
      string languageJson = languageAsset.text;

      Anki.AppResources.StringTable st = Anki.AppResources.StringTable.LoadStringsFromSmartlingJSONFile(languageJson);
      _st.MergeEntriesFromStringTable(st);
    }
  }

  public static System.Globalization.CultureInfo GetCultureInfo() {
    return _CurrentCulture;
  }
}