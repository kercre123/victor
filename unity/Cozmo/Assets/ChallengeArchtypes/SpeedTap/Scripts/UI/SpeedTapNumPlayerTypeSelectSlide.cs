using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System.Collections;
using Anki.UI;

namespace SpeedTap {
  public class SpeedTapNumPlayerTypeSelectSlide : MonoBehaviour {

    [SerializeField]
    private Button _HumanVCozmoBtn;

    [SerializeField]
    private Button _HumanVHumanVCozmoBtn;

    [SerializeField]
    private Button _CozmoVAutomationBtn;

    [SerializeField]
    private Button _HumanVHumanBtn;

    private UnityAction _Callback = null;

    private GameBase _Game;

    public void Init(UnityAction callback, SpeedTapGame game) {
      _Callback = callback;
      _Game = game;
      game.ClearPlayers(); // Clears the defaults
      _HumanVCozmoBtn.onClick.AddListener(() => {
        game.AddPlayer(PlayerType.Cozmo, Localization.Get(LocalizationKeys.kNameCozmo));
        game.AddPlayer(PlayerType.Human, DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
        StartGame();
      });

      _HumanVHumanVCozmoBtn.onClick.AddListener(() => {
        game.AddPlayer(PlayerType.Cozmo, Localization.Get(LocalizationKeys.kNameCozmo));
        game.AddPlayer(PlayerType.Human, DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
        game.AddPlayer(PlayerType.Human, Localization.Get(LocalizationKeys.kNamePlayer));
        StartGame();
      });

      _CozmoVAutomationBtn.onClick.AddListener(() => {
        game.AddPlayer(PlayerType.Cozmo, Localization.Get(LocalizationKeys.kNameCozmo));
        game.AddPlayer(PlayerType.Automation, "Automation");
        StartGame();
      });

      _HumanVHumanBtn.onClick.AddListener(() => {
        game.AddPlayer(PlayerType.Human, "DebugHuman1");
        game.AddPlayer(PlayerType.Human, "DebugHuman2");
        StartGame();
      });
    }

    private void StartGame() {
      _Game.InitializeAllPlayers();
      if (_Callback != null) {
        _Callback();
      }
    }

  }
}