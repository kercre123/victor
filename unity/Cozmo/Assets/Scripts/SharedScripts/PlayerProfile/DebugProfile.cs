using System.Collections.Generic;
using Anki.Debug;

namespace DataPersistence {
  // Sticky props that will be compiled out of final version go here.
  // Since some of the debug menu comes from engine and in most cases we don't want it to stick so it's easily resetable
  // things that go here should be the exceptions.
  [System.Serializable]
  public class DebugProfile {
    public static string kNoFreelpayOnStartLock = "no_freeplay_on_start";
    public PerfWarningDisplay.PerfWarningDisplayMode PerfInfoDisplayMode;
    public bool DebugPauseEnabled;
    public bool NoFreeplayOnStart;
    public bool ShowDroneModeDebugInfo;
    public bool UseFastConnectivityFlow;
    public bool OverrideLanguage;
    public UnityEngine.SystemLanguage LanguageSettingOverride;
    public bool UseConnectFlowInMock;
    public bool UseAndroidFlowInMock;
    public bool ForceFirstTimeConnectFlow;
    public bool ForceNotFirstTimeConnectFlow;
    public bool FakeGermanLocale;
    public bool EnableAutoBlockPoolOnStart;
    public bool LoadTestCodeLabProjects;
    public bool DisplayPerfDataInCodeLab;
    public bool ShowAllCodeLabFeaturedContent;

    public DebugProfile() {
      PerfInfoDisplayMode = PerfWarningDisplay.PerfWarningDisplayMode.TurnsOnWhenWarning;
      DebugPauseEnabled = false;
      NoFreeplayOnStart = false;
      ShowDroneModeDebugInfo = false;
      UseConnectFlowInMock = false;
      UseAndroidFlowInMock = false;
      ForceFirstTimeConnectFlow = false;
      ForceNotFirstTimeConnectFlow = false;
      FakeGermanLocale = false;
      EnableAutoBlockPoolOnStart = true;
      LoadTestCodeLabProjects = false;
      DisplayPerfDataInCodeLab = false;
      ShowAllCodeLabFeaturedContent = false;

      DebugConsoleData.Instance.AddConsoleVar("NoFreeplayOnStart", "Animator", this);
      DebugConsoleData.Instance.AddConsoleVar("EnableAutoBlockPoolOnStart", "Animator", this);
      DebugConsoleData.Instance.AddConsoleVar("UseFastConnectivityFlow", "QA", this);
      // editor-only ui debug options
#if UNITY_EDITOR 
      DebugConsoleData.Instance.AddConsoleVar("UseConnectFlowInMock", "UIMockFlow", this);
      DebugConsoleData.Instance.AddConsoleVar("UseAndroidFlowInMock", "UIMockFlow", this);
      DebugConsoleData.Instance.AddConsoleVar("ForceFirstTimeConnectFlow", "UIMockFlow", this);
      DebugConsoleData.Instance.AddConsoleVar("ForceNotFirstTimeConnectFlow", "UIMockFlow", this);
      DebugConsoleData.Instance.AddConsoleVar("FakeGermanLocale", "UIMockFlow", this);
#endif
      DebugConsoleData.Instance.AddConsoleVar("LoadTestCodeLabProjects", "CodeLab", this);
      DebugConsoleData.Instance.AddConsoleVar("DisplayPerfDataInCodeLab", "CodeLab", this);
      DebugConsoleData.Instance.AddConsoleVar("ShowAllCodeLabFeaturedContent", "CodeLab", this);

      DebugConsoleData.Instance.AddConsoleFunction("UseSystemSettings", "Language", (str) => {
        OverrideLanguage = false;
        LanguageSettingOverride = UnityEngine.SystemLanguage.English;
        DataPersistence.DataPersistenceManager.Instance.Save();
      });
      DebugConsoleData.Instance.AddConsoleFunction("UseEnglish", "Language", (str) => {
        OverrideLanguage = true;
        LanguageSettingOverride = UnityEngine.SystemLanguage.English;
        DataPersistence.DataPersistenceManager.Instance.Save();
      });
      DebugConsoleData.Instance.AddConsoleFunction("UseFrench", "Language", (str) => {
        OverrideLanguage = true;
        LanguageSettingOverride = UnityEngine.SystemLanguage.French;
        DataPersistence.DataPersistenceManager.Instance.Save();
      });
      DebugConsoleData.Instance.AddConsoleFunction("UseGerman", "Language", (str) => {
        OverrideLanguage = true;
        LanguageSettingOverride = UnityEngine.SystemLanguage.German;
        DataPersistence.DataPersistenceManager.Instance.Save();
      });
      DebugConsoleData.Instance.AddConsoleFunction("UseJapanese", "Language", (str) => {
        OverrideLanguage = true;
        LanguageSettingOverride = UnityEngine.SystemLanguage.Japanese;
        DataPersistence.DataPersistenceManager.Instance.Save();
      });

      DebugConsoleData.Instance.DebugConsoleVarUpdated += HandleDebugConsoleVarUpdated;
    }

    private void HandleDebugConsoleVarUpdated(string varName) {
      switch (varName) {
      case "NoFreeplayOnStart":
        DataPersistence.DataPersistenceManager.Instance.Save();
        if (RobotEngineManager.Instance != null && RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayActivity(!NoFreeplayOnStart);
        }
        break;
      case "UseFastConnectivityFlow":
      case "EnableAutoBlockPoolOnStart":
      case "LoadTestCodeLabProjects":
      case "DisplayPerfDataInCodeLab":
      case "ShowAllCodeLabFeaturedContent":
        DataPersistence.DataPersistenceManager.Instance.Save();
        break;
      }
    }
  }
}
