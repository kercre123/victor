using UnityEngine;
using System.Collections.Generic;
using System;
using System.IO;

public static class Localization {

  private static Anki.AppResources.StringTable _st = new Anki.AppResources.StringTable();

  public static bool showDebugLocText = false;
  public const string kNewLine = "\n";

  public static string Get(string key) {
    string value;
    if (!_st.TryGetString(key, out value)) {
#if !UNITY_EDITOR
      //disabling populating of key name for COPPA reasons (JDN)
      DAS.Warn("LocalizedString.Get.MissingKey", PrivacyGuard.HidePersonallyIdentifiableInfo(key));
#else
      if (Application.isPlaying) {
        DAS.Warn("LocalizedString.Get.MissingKey", key);
      }
#endif
      value = key;
    }
    else if (showDebugLocText) {
      value = "@" + key;
    }

    return value;
  }

  public static string GetNumber(int number) {
    return string.Format(GetCultureInfo(), "{0:N0}", number);
  }

  public static bool IsLocalizationKey(string textParam) {
    return ((!string.IsNullOrEmpty(textParam)) && (textParam.Length > 1) && ('#' == textParam[0]));
  }

  public static string GetWithArgs(string localizationKey, params object[] args) {
    string localized = Get(localizationKey);
    localized = string.Format(GetCultureInfo(), localized, args);
    return localized;
  }

  public static string GetAmountName(string localizationKey, int amount) {
    return (amount == 1) ? GetSingularName(localizationKey) : GetPluralName(localizationKey);
  }

  public static string GetSingularName(string localizationKey) {
    return Localization.Get(localizationKey + ".singular");
  }

  public static string GetPluralName(string localizationKey) {
    return Localization.Get(localizationKey + ".plural");
  }

  public static string GetCountLabel(string localizationKey, int amount) {
    return GetWithArgs(LocalizationKeys.kLabelSimpleCount, GetNumber(amount), GetAmountName(localizationKey, amount));
  }

  private static string _CurrentLocale = null;
  private static string _CurrentFontBundleVariant = null;
  private static System.Globalization.CultureInfo _CurrentCulture = null;

  public static string LoadLocaleAndCultureInfo(bool overrideLanguage = false, SystemLanguage languageOverride = SystemLanguage.English) {

#if SHIPPING
     _CurrentLocale = "en-US";
     _CurrentFontBundleVariant = "latin";
#else
    // Only show localization logic in non-shipping builds
    SystemLanguage language = Application.systemLanguage;
    if (overrideLanguage) {
      language = languageOverride;
    }

    switch (language) {
    case SystemLanguage.English:
      _CurrentLocale = "en-US";
      _CurrentFontBundleVariant = "latin";
      break;
    case SystemLanguage.French:
      _CurrentLocale = "fr-FR";
      _CurrentFontBundleVariant = "latin";
      break;
    case SystemLanguage.German:
      _CurrentLocale = "de-DE";
      _CurrentFontBundleVariant = "latin";
      break;
    case SystemLanguage.Japanese:
      _CurrentLocale = "ja-JP";
      _CurrentFontBundleVariant = "ja-JP";
      break;
    default:
      _CurrentLocale = "en-US";
      _CurrentFontBundleVariant = "latin";
      break;
    }

#endif

    _CurrentCulture = new System.Globalization.CultureInfo(_CurrentLocale);
    return _CurrentLocale;
  }

  // Return locale code for a language code.
  public static string GetStringsLocale() {
    if (string.IsNullOrEmpty(_CurrentLocale)) {
      LoadLocaleAndCultureInfo();
    }

    return _CurrentLocale;
  }

  public static string GetCurrentFontBundleVariant() {
    if (string.IsNullOrEmpty(_CurrentFontBundleVariant)) {
      LoadLocaleAndCultureInfo();
    }
    return _CurrentFontBundleVariant;
  }

  private const string kLocalizationStreamingAssetsFolderPath = "/LocalizedStrings/";

  public static void LoadStrings() {
    // For each localization file in the locale's directory
    string locale = GetStringsLocale();
    foreach (var filePath in GetLocalizationJsonFilePaths(locale)) {

      // Load the localization into a string table so that we can query it at runtime
      JSONObject languageJson = GetJsonContentsFromLocalizationFile(locale, filePath);

      Anki.AppResources.StringTable st = Anki.AppResources.StringTable.LoadStringsFromSmartlingJSONFile(languageJson);
      _st.MergeEntriesFromStringTable(st);
    }
  }

  public static System.Globalization.CultureInfo GetCultureInfo() {
    return _CurrentCulture;
  }

  public static string[] GetLocalizationJsonFilePaths(string locale) {
#if UNITY_EDITOR
    return Directory.GetFiles(Application.streamingAssetsPath + kLocalizationStreamingAssetsFolderPath + locale, "*.json");
#else
    return Directory.GetFiles(PlatformUtil.GetResourcesBaseFolder() + kLocalizationStreamingAssetsFolderPath + locale, "*.json");
#endif
  }

  public static string[] GetLocalizationAllLanguagesPaths() {
#if UNITY_EDITOR
    return Directory.GetDirectories(Application.streamingAssetsPath + kLocalizationStreamingAssetsFolderPath);
#else
    return Directory.GetDirectories(PlatformUtil.GetResourcesBaseFolder() + kLocalizationStreamingAssetsFolderPath);
#endif
  }

  public static JSONObject GetJsonContentsFromLocalizationFile(string locale, string localizationFilePath) {
    string languageJson = File.ReadAllText(localizationFilePath);
    return new JSONObject(languageJson);
  }

  public static string ReadLocalizedTextFromFile(string fileName) {
    string currentLocale = GetStringsLocale();
#if UNITY_EDITOR || UNITY_IOS
    string filePath = Application.streamingAssetsPath + kLocalizationStreamingAssetsFolderPath + currentLocale + "/" + fileName;
#elif UNITY_ANDROID
    string filePath = PlatformUtil.GetResourcesBaseFolder() + kLocalizationStreamingAssetsFolderPath + currentLocale + "/" + fileName;
#endif
    return File.ReadAllText(filePath);
  }
}