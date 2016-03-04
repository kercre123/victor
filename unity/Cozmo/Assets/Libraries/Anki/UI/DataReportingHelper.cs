using UnityEngine;

namespace Anki {
  namespace UI {
    public static class DataReportingHelper {
      
      public static bool IsEnabled() {
        return !AnkiPrefs.HasKey(AnkiPrefs.Prefs.DataReportingEnabled)
          || AnkiPrefs.GetInt(AnkiPrefs.Prefs.DataReportingEnabled) > 0;
      }
      
      public static void SetIsEnabled(bool enabled) {
        AnkiPrefs.SetInt(AnkiPrefs.Prefs.DataReportingEnabled, enabled ? 1 : 0);
        if (enabled) {
          EnableReporting();
        }
        else {
          DisableReporting();
        }
      }

      private static void EnableReporting() {
        DAS.Debug("DataReportingHelper.EnableReporting", "enabling data reporting");
        // TODO: Something
        //DAS.Enable(DAS.DisableNetworkReason_UserOptOut);
      }

      private static void DisableReporting() {
        DAS.Debug("DataReportingHelper.DisableReporting", "disabling data reporting");
        // TODO: Something
        //DAS.Disable(DAS.DisableNetworkReason_UserOptOut);
      }
      
      public static void OnApplicationStarted() {
        bool enabled = IsEnabled();
        DAS.Info("DataReportingHelper.OnApplicationStarted", "reporting is: " + enabled);
        // this stuff defaults to on, so turn it off if not enabled
        if (!enabled) {
          DisableReporting();
        }
      }
      
    }
  }
}
