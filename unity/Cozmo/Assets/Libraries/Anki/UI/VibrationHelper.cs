using UnityEngine;

namespace Anki {
  namespace UI {
    public static class VibrationHelper {

      public static bool IsEnabled() {
        if (!AnkiPrefs.HasKey(AnkiPrefs.Prefs.VibrationEnabled)) {
          // initialize default
          SetIsEnabled(true);
          return true;
        }
        else {
          return AnkiPrefs.GetInt(AnkiPrefs.Prefs.VibrationEnabled) > 0;
        }
      }

      public static void SetIsEnabled(bool enabled) {
        AnkiPrefs.SetInt(AnkiPrefs.Prefs.VibrationEnabled, enabled ? 1 : 0);
        AnkiPrefs.Save();
      }

      public static void Vibrate() {
        if (IsEnabled()) {
          #if UNITY_ANDROID || UNITY_IOS
          Handheld.Vibrate();
          #endif
        }
      }

    }
  }
}
