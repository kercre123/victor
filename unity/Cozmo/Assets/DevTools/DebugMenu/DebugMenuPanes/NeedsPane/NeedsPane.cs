using UnityEngine;
using System.Linq;
using UnityEngine.UI;
using Anki.Cozmo.ExternalInterface;

public class NeedsPane : MonoBehaviour {

  [SerializeField]
  private Toggle _NeedsEnabledCheckbox;

  [SerializeField]
  private Dropdown _NeedsActionCompletedDropDown;
  [SerializeField]
  private Button _NeedsActionApplyButton;

  [SerializeField]
  private Button _GiveStarButton;

  [SerializeField]
  private InputField _LevelEnergyInput;
  [SerializeField]
  private InputField _LevelPlayInput;
  [SerializeField]
  private InputField _LevelRepairInput;

  [SerializeField]
  private Button _LevelApplyButton;

  [SerializeField]
  private InputField _PassTimeInput;
  [SerializeField]
  private Button _PassTimeApplyButton;

  // Shortcut for  CONSOLE_VAR names
  private const string _kDebugGiveStarKey = "DebugGiveStar";
  private const string _kDebugPassTimeMinutesKey = "DebugPassTimeMinutes";
  private const string _kDebugCompleteActionKey = "DebugCompleteAction";

  private const string _kDebugSetEnergyLevelKey = "DebugSetEnergyLevel";
  private const string _kDebugSetPlayLevelKey = "DebugSetPlayLevel";
  private const string _kDebugSetRepairLevelKey = "DebugSetRepairLevel";

  private const string _kUseNeedsManagerKey = "UseNeedManager";
  private const string _kUseNeedsDefaultUnlocksKey = "UseNeedsDefaultUnlocks";
  private const string _kSaveConsoleVarsKey = "SaveConsoleVars";

  // Use this for initialization
  void Start() {
    _GiveStarButton.onClick.AddListener(HandleGiveStarTap);
    _PassTimeApplyButton.onClick.AddListener(HandlePassTimeTap);
    _NeedsActionApplyButton.onClick.AddListener(HandleNeedsActionCompleteTap);
    _PassTimeInput.text = "1";

    _LevelApplyButton.onClick.AddListener(HandleSetNeedsMeterLevelsTap);

    _NeedsActionCompletedDropDown.ClearOptions();
    var needsActionArray = System.Enum.GetNames(typeof(Anki.Cozmo.NeedsActionId)).ToList();
    _NeedsActionCompletedDropDown.AddOptions(needsActionArray);

    RobotEngineManager.Instance.AddCallback<NeedsState>(HandleNeedsStateFromEngine);
    RobotEngineManager.Instance.Message.GetNeedsState = new GetNeedsState();
    RobotEngineManager.Instance.SendMessage();

    // Really a little more than just could get toggled, but most people will just toggle from this menu as a group.
    _NeedsEnabledCheckbox.isOn = DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.UseNeedsHub;
    _NeedsEnabledCheckbox.onValueChanged.AddListener(HandleEnableChanged);
  }

  void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<NeedsState>(HandleNeedsStateFromEngine);
  }

  private void HandleEnableChanged(bool isOn) {
    DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.UseNeedsHub = isOn;
    RobotEngineManager.Instance.SetDebugConsoleVar(_kUseNeedsManagerKey, isOn.ToString());
    RobotEngineManager.Instance.SetDebugConsoleVar(_kUseNeedsDefaultUnlocksKey, isOn.ToString());
    DataPersistence.DataPersistenceManager.Instance.Save();
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kSaveConsoleVarsKey, "");
  }

  private void HandleNeedsStateFromEngine(NeedsState newNeedsState) {
    _LevelEnergyInput.text = newNeedsState.curNeedLevel[(int)Anki.Cozmo.NeedId.Energy].ToString("N2");
    _LevelPlayInput.text = newNeedsState.curNeedLevel[(int)Anki.Cozmo.NeedId.Play].ToString("N2");
    _LevelRepairInput.text = newNeedsState.curNeedLevel[(int)Anki.Cozmo.NeedId.Repair].ToString("N2");
    // Remove so that it doesn't change past first init
    RobotEngineManager.Instance.RemoveCallback<NeedsState>(HandleNeedsStateFromEngine);
  }

  private void HandleGiveStarTap() {
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugGiveStarKey, "");
  }
  private void HandlePassTimeTap() {
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugPassTimeMinutesKey, _PassTimeInput.text);
  }
  private void HandleNeedsActionCompleteTap() {
    string actionSelected = _NeedsActionCompletedDropDown.options[_NeedsActionCompletedDropDown.value].text;
    try {
      Anki.Cozmo.NeedsActionId actionEnum = (Anki.Cozmo.NeedsActionId)System.Enum.Parse(typeof(Anki.Cozmo.NeedsActionId), actionSelected);
      RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugCompleteActionKey, actionEnum.ToString());
    }
    catch {
    }
  }

  private void HandleSetNeedsMeterLevelsTap() {
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugSetEnergyLevelKey, _LevelEnergyInput.text);
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugSetPlayLevelKey, _LevelPlayInput.text);
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugSetRepairLevelKey, _LevelRepairInput.text);
  }
}
