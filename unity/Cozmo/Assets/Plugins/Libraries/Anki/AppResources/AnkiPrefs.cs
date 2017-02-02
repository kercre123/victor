using System.Collections.Generic;
using UnityEngine;

// Wraps Unity's PlayerPrefs class with an interface that allows us to selectively
// reset individual settings from a centralized place
public static class AnkiPrefs {

  // list of preferences used by the game
  // if you add something here, YOU MUST REGISTER IT IN THE STATIC CONSTRUCTOR (scroll down)
  // (otherwise app initialization will throw an exception)
  public enum Prefs {
    AudioVolumeMusic,
    AudioVolumeSfx,
    AudioVolumeVoice,
    AudioVolumeThrottle,
    DataReportingEnabled,
    VibrationEnabled,
    DevFeaturesEnabled,
    HomeScreenTournamentButtonPressed,
    LastSelectedScene,
    OnboardingEnabled,
    HasPassedFTUE,
    OpenPlayVisitCount,
    CurrentGameId,
    SeenJumpPiece,
    HasSelectedCharacter,
    ShownTournamentGen1Vehicles,
    ShownAddPlayer,
    AndroidAutoConnectDisabled
  }

  public enum PrefType {
    Invalid,
    Int,
    String,
    Float
  }

  static AnkiPrefs() {
    // set base values for all prefs so we can check later
    foreach (Prefs p in System.Enum.GetValues(typeof(Prefs))) {
      _typeMap[p] = PrefType.Invalid;
    }
    RegisterPref(Prefs.AudioVolumeMusic, "KEY_MUSIC_VOLUME", PrefType.Int);
    RegisterPref(Prefs.AudioVolumeSfx, "KEY_SFX_VOLUME", PrefType.Int);
    RegisterPref(Prefs.AudioVolumeVoice, "KEY_VOICE_VOLUME", PrefType.Int);
    RegisterPref(Prefs.AudioVolumeThrottle, "KEY_VEHICLE_THROTTLE_VOLUME", PrefType.Int);
    RegisterPref(Prefs.DataReportingEnabled, "REPORTING_ENABLED", PrefType.Int);
    RegisterPref(Prefs.VibrationEnabled, "VIBRATION_ENABLED", PrefType.Int);
    RegisterPref(Prefs.DevFeaturesEnabled, "AreDevFeaturesEnabled", PrefType.Int);
    RegisterPref(Prefs.HomeScreenTournamentButtonPressed, "HomeScreenTournamentButtonPressed", PrefType.Int);
    RegisterPref(Prefs.LastSelectedScene, "LastSelectedScene", PrefType.String);
    RegisterPref(Prefs.OnboardingEnabled, "OnboardingEnabled", PrefType.Int);
    RegisterPref(Prefs.HasPassedFTUE, "HAS_PASSED_FTUE", PrefType.Int);
    RegisterPref(Prefs.OpenPlayVisitCount, "open_play_visit_count", PrefType.Int);
    RegisterPref(Prefs.CurrentGameId, "GameId", PrefType.String);
    RegisterPref(Prefs.SeenJumpPiece, "SeenJumpPiece", PrefType.Int);
    RegisterPref(Prefs.HasSelectedCharacter, "CharacterSelected", PrefType.Int);
    RegisterPref(Prefs.ShownTournamentGen1Vehicles, "DidShowTournamentGen1VehicleModal", PrefType.Int);
    RegisterPref(Prefs.ShownAddPlayer, "DidShowAddPlayer", PrefType.Int);
    RegisterPref(Prefs.AndroidAutoConnectDisabled, "AndroidAutoConnectDISABLED", PrefType.Int);

    // verify that every pref was registered
    foreach (Prefs p in System.Enum.GetValues(typeof(Prefs))) {
      if (_typeMap[p] == PrefType.Invalid) {
        throw new System.InvalidOperationException("pref type " + p.ToString() + " was not registered");
      }
    }
  }

  public static string GetKey(Prefs prefType) {
    return _keyMap[prefType];
  }

  public static PrefType GetType(Prefs prefType) {
    return _typeMap[prefType];
  }

  public static void Save() {
    PlayerPrefs.Save();
  }

  public static bool HasKey(Prefs prefType) {
    return PlayerPrefs.HasKey(GetKey(prefType));
  }

  public static void DeleteKey(Prefs prefType) {
    PlayerPrefs.DeleteKey(GetKey(prefType));
  }

  public static void DeleteAll() {
    PlayerPrefs.DeleteAll();
  }

  public static int GetInt(Prefs prefType, int defaultValue = 0) {
    CheckType(prefType, PrefType.Int);
    return PlayerPrefs.GetInt(GetKey(prefType), defaultValue);
  }

  public static string GetString(Prefs prefType, string defaultValue = "") {
    CheckType(prefType, PrefType.String);
    return PlayerPrefs.GetString(GetKey(prefType), defaultValue);
  }

  public static float GetFloat(Prefs prefType, float defaultValue = 0.0f) {
    CheckType(prefType, PrefType.Float);
    return PlayerPrefs.GetFloat(GetKey(prefType), defaultValue);
  }

  public static object GetObject(Prefs prefType) {
    if (HasKey(prefType)) {
      switch (GetType(prefType)) {
        case PrefType.Invalid: return null;
        case PrefType.Int:     return GetInt(prefType);
        case PrefType.String:  return GetString(prefType);
        case PrefType.Float:   return GetFloat(prefType);
      }
      // shouldn't get here? but compiler whines if it isn't here
      return null;
    }
    else {
      return null;
    }
  }

  public static bool TryGetInt(Prefs prefType, out int outValue) {
    CheckType(prefType, PrefType.Int);
    if (HasKey(prefType)) {
      outValue = GetInt(prefType, 0);
      return true;
    }
    else {
      outValue = 0;
      return false;
    }
  }

  public static bool TryGetString(Prefs prefType, out string outValue) {
    CheckType(prefType, PrefType.String);
    if (HasKey(prefType)) {
      outValue = GetString(prefType, "");
      return true;
    }
    else {
      outValue = "";
      return false;
    }
  }

  public static bool TryGetFloat(Prefs prefType, out float outValue) {
    CheckType(prefType, PrefType.Float);
    if (HasKey(prefType)) {
      outValue = GetFloat(prefType, 0.0f);
      return true;
    }
    else {
      outValue = 0.0f;
      return false;
    }
  }

  public static void SetInt(Prefs prefType, int value) {
    CheckType(prefType, PrefType.Int);
    PlayerPrefs.SetInt(GetKey(prefType), value);
  }

  public static void SetString(Prefs prefType, string value) {
    CheckType(prefType, PrefType.String);
    PlayerPrefs.SetString(GetKey(prefType), value);
  }

  public static void SetFloat(Prefs prefType, float value) {
    CheckType(prefType, PrefType.Float);
    PlayerPrefs.SetFloat(GetKey(prefType), value);
  }

  private static Dictionary<Prefs, PrefType> _typeMap = new Dictionary<Prefs, PrefType>();
  private static Dictionary<Prefs, string> _keyMap = new Dictionary<Prefs, string>();

  private static void RegisterPref(Prefs pref, string key, PrefType type) {
    if (_typeMap[pref] != PrefType.Invalid) {
      throw new System.InvalidOperationException("pref " + pref.ToString() + " has already been registered");
    }
    _typeMap[pref] = type;
    _keyMap[pref] = key;
  }

  private static void CheckType(Prefs pref, PrefType type) {
    PrefType existingType = _typeMap[pref];
    if (existingType != type) {
      throw new System.InvalidCastException("pref " + pref.ToString() + " exists as type "
                                              + existingType.ToString() + " but attempting to use it as " + type.ToString());
    }
  }
}
