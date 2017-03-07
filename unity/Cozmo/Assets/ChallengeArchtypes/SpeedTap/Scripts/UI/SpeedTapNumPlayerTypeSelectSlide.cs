using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System.Collections;
using Cozmo.UI;

namespace SpeedTap {
  public class SpeedTapNumPlayerTypeSelectSlide : MonoBehaviour {

    [SerializeField]
    private CozmoButtonLegacy _HumanVCozmoBtn;

    [SerializeField]
    private CozmoButtonLegacy _HumanVHumanVCozmoBtn;

    [SerializeField]
    private GameObject _LockOverlay;

    private UnityAction _Callback = null;

    private SpeedTapGame _Game;

    public void Init(UnityAction callback, SpeedTapGame game) {
      _Callback = callback;
      _Game = game;
      game.ClearPlayers(); // Clears the defaults
      _HumanVCozmoBtn.Initialize(Handle2PlayerClicked, "btn_speedtap_single_player", _Game.ChallengeID);
      _HumanVHumanVCozmoBtn.Initialize(HandleMPClicked, "btn_speedtap_multi_player", _Game.ChallengeID);


      if (_LockOverlay != null) {
        _LockOverlay.SetActive(!IsMPModeUnlocked());
      }

      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("StartAutomation", "SpeedTap",
                                  (string str) => {
                                    _Game.AddPlayer(PlayerType.Cozmo, Localization.Get(LocalizationKeys.kNameCozmo), Color.white);
                                    _Game.AddPlayer(PlayerType.Automation, "Automation", Color.white);
                                    StartGame();
                                  });
    }

    public void OnDestroy() {
      DAS.Event("game.numplayers", _Game.GetPlayerCount().ToString(), DASUtil.FormatExtraData(_Game.CurrentDifficulty.ToString()));

      Anki.Debug.DebugConsoleData.Instance.RemoveConsoleData("StartAutomation", "SpeedTap");
    }

    // Unlike Difficulty MP just requires playing a game, not beating it...
    public bool IsMPModeUnlocked() {
      DataPersistence.PlayerProfile playerProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
      // Scores are per mode
      string key = _Game.ChallengeID + _Game.CurrentDifficulty;
      return playerProfile.HighScores.ContainsKey(key);
    }

    private void Handle2PlayerClicked() {
      _Game.AddPlayer(PlayerType.Cozmo, Localization.Get(LocalizationKeys.kNameCozmo), Color.white);
      _Game.AddPlayer(PlayerType.Human, DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName, Color.white);
      StartGame();
    }

    private void HandleMPClicked() {
      if (IsMPModeUnlocked()) {
        // Requires 3 cubes, not two to play this mode...
        int neededCubes = 3;
        if (_Game.CurrentRobot.LightCubes.Count >= neededCubes) {

          _Game.AddPlayer(PlayerType.Cozmo, Localization.Get(LocalizationKeys.kNameCozmo), _Game.GameConfig.CozmoTint);
          _Game.AddPlayer(PlayerType.Human, Localization.Get(LocalizationKeys.kSpeedTapMultiplayerPlayer1), _Game.GameConfig.Player1Tint);
          _Game.AddPlayer(PlayerType.Human, Localization.Get(LocalizationKeys.kSpeedTapMultiplayerPlayer2), _Game.GameConfig.Player2Tint);
          StartGame();
        }
        else {
          int differenceCubes = neededCubes - _Game.CurrentRobot.LightCubes.Count;
          object[] descLocArgs = new object[] {
              differenceCubes,
              (Cozmo.ItemDataConfig.GetCubeData().GetAmountName(differenceCubes)),
              Localization.Get(LocalizationKeys.kSpeedTapTitle)
            };
          AlertModalData notEnoughCubesData = new AlertModalData("speedtap_mp_not_enough_cubes_alert",
                                                                 LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalTitle,
                                                                  LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalDescription,
                                                                 new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                                                 descLocArgs: descLocArgs);
          UIManager.OpenAlert(notEnoughCubesData, new ModalPriorityData(ModalPriorityLayer.Low, 0, LowPriorityModalAction.Queue, HighPriorityModalAction.Queue));
        }
      }
      else {
        // Throw up the warning
        AlertModalData lockedData = new AlertModalData("speedtap_mp_locked_alert",
                                                       LocalizationKeys.kSpeedTapTitleMPLocked,
                                                       LocalizationKeys.kSpeedTapTextMPLocked,
                                                       new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));
        ModalPriorityData lockedPriority = new ModalPriorityData(ModalPriorityLayer.Low, 0, LowPriorityModalAction.Queue, HighPriorityModalAction.Queue);
        UIManager.OpenAlert(lockedData, lockedPriority);
      }
    }

    private void StartGame() {
      _Game.InitializeAllPlayers();
      if (_Callback != null) {
        _Callback();
      }
    }

  }
}