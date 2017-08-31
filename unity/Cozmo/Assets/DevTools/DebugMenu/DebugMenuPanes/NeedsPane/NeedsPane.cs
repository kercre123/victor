using Anki.Cozmo.ExternalInterface;
using Cozmo.Needs;
using UnityEngine;
using UnityEngine.UI;
using System.Linq;

public class NeedsPane : MonoBehaviour {

  [SerializeField]
  private Dropdown _NeedsActionCompletedDropDown;
  [SerializeField]
  private Button _NeedsActionApplyButton;

  [SerializeField]
  private Button _GiveStarButton;

  [SerializeField]
  private Button _ShowRewardDialogButton;

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

  [SerializeField]
  private Text _ABTestText;

  // Shortcut for  CONSOLE_VAR names
  private const string _kDebugGiveStarKey = "DebugGiveStar";
  private const string _kDebugPassTimeMinutesKey = "DebugPassTimeMinutes";
  private const string _kDebugCompleteActionKey = "DebugCompleteAction";

  private const string _kDebugSetEnergyLevelKey = "DebugSetEnergyLevel";
  private const string _kDebugSetPlayLevelKey = "DebugSetPlayLevel";
  private const string _kDebugSetRepairLevelKey = "DebugSetRepairLevel";

  // Use this for initialization
  void Start() {
    _GiveStarButton.onClick.AddListener(HandleGiveStarTap);
    _ShowRewardDialogButton.onClick.AddListener(HandleShowRewardDialogTap);

    _LevelApplyButton.onClick.AddListener(HandleSetNeedsMeterLevelsTap);

    _PassTimeApplyButton.onClick.AddListener(HandlePassTimeTap);
    _PassTimeInput.text = "1";

    _NeedsActionCompletedDropDown.ClearOptions();
    var needsActionArray = System.Enum.GetNames(typeof(Anki.Cozmo.NeedsActionId)).ToList();
    _NeedsActionCompletedDropDown.AddOptions(needsActionArray);
    _NeedsActionApplyButton.onClick.AddListener(HandleNeedsActionCompleteTap);

    RobotEngineManager.Instance.AddCallback<NeedsState>(HandleNeedsStateFromEngine);
    RobotEngineManager.Instance.Message.GetNeedsState = new GetNeedsState();
    RobotEngineManager.Instance.SendMessage();
  }

  void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<NeedsState>(HandleNeedsStateFromEngine);
  }

  private void HandleNeedsStateFromEngine(NeedsState newNeedsState) {
    _LevelEnergyInput.text = newNeedsState.curNeedLevel[(int)Anki.Cozmo.NeedId.Energy].ToString("N6");
    _LevelPlayInput.text = newNeedsState.curNeedLevel[(int)Anki.Cozmo.NeedId.Play].ToString("N6");
    _LevelRepairInput.text = newNeedsState.curNeedLevel[(int)Anki.Cozmo.NeedId.Repair].ToString("N6");
    _ABTestText.text = newNeedsState.unconnectedDecayTestVariation;
    // Remove so that it doesn't change past first init
    RobotEngineManager.Instance.RemoveCallback<NeedsState>(HandleNeedsStateFromEngine);
  }

  private void HandleGiveStarTap() {
    RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugGiveStarKey, "");
    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
      MessageEngineToGame messageEngineToGame = new MessageEngineToGame();
      StarUnlocked starCompletedMsg = new StarUnlocked();
      starCompletedMsg.Initialize(1, 3, 1);
      messageEngineToGame.StarUnlocked = starCompletedMsg;
      RobotEngineManager.Instance.MockCallback(messageEngineToGame);
    }
  }

  private void HandleShowRewardDialogTap() {
    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
      MessageEngineToGame messageEngineToGame = new MessageEngineToGame();
      StarLevelCompleted starLevelCompleted = new StarLevelCompleted();
      Anki.Cozmo.NeedsReward[] needsRewards = new Anki.Cozmo.NeedsReward[3];
      // Test one and two digit numbers
      needsRewards[0] = new Anki.Cozmo.NeedsReward(Anki.Cozmo.NeedsRewardType.Sparks,
                                                   Random.Range(9, 11).ToString(),
                                                   inventoryIsFull: false);
      // Test game and trick unlocks
      Anki.Cozmo.UnlockId newUnlock = (Random.Range(0f, 1f) > 0.5f)
        ? Anki.Cozmo.UnlockId.QuickTapGame : Anki.Cozmo.UnlockId.RollCube;
      needsRewards[1] = new Anki.Cozmo.NeedsReward(Anki.Cozmo.NeedsRewardType.Unlock,
                                                   newUnlock.ToString(),
                                                   inventoryIsFull: false);
      needsRewards[2] = new Anki.Cozmo.NeedsReward(Anki.Cozmo.NeedsRewardType.Song,
                                                   Anki.Cozmo.UnlockId.Singing_TakeMeOutToTheBallgame.ToString(),
                                                   inventoryIsFull: false);
      starLevelCompleted.Initialize(1, 3, needsRewards);
      messageEngineToGame.StarLevelCompleted = starLevelCompleted;
      RobotEngineManager.Instance.MockCallback(messageEngineToGame);
    }
    else {
      int starsToGrant = NeedsStateManager.Instance.GetLatestStarForNextUnlockFromEngine()
                                          - NeedsStateManager.Instance.GetLatestStarAwardedFromEngine();
      for (int i = 0; i < starsToGrant; i++) {
        RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_kDebugGiveStarKey, "");
      }
    }
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
