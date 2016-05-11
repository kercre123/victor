using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Cozmo.HomeHub;
using System.Linq;

public class MinigamePane : MonoBehaviour {
  private HomeHub _currHub = null;
  // Force Win Game Buttons
  [SerializeField]
  private Button _CozmoGameButton;
  [SerializeField]
  private Button _PlayerGameButton;

  // Force Win Round Buttons
  [SerializeField]
  private Button _CozmoRoundButton;
  [SerializeField]
  private Button _PlayerRoundButton;

  // Force Rounds Won Buttons/Fields
  [SerializeField]
  private Button _SetCozmoRoundsButton;
  [SerializeField]
  private Button _SetPlayerRoundsButton;
  [SerializeField]
  private InputField _SetCozmoRoundsField;
  [SerializeField]
  private InputField _SetPlayerRoundsField;

  // Force Score Buttons/Fields
  [SerializeField]
  private Button _SetCozmoScoreButton;
  [SerializeField]
  private Button _SetPlayerScoreButton;
  [SerializeField]
  private InputField _SetCozmoScoreField;
  [SerializeField]
  private InputField _SetPlayerScoreField;

  // Use this for initialization
  void Start() {
    _CozmoGameButton.onClick.AddListener(CozmoWinGameClicked);
    _CozmoRoundButton.onClick.AddListener(CozmoRoundButtonClicked);
    _SetCozmoRoundsButton.onClick.AddListener(CozmoSetRoundsButtonClicked);
    _SetCozmoScoreButton.onClick.AddListener(CozmoSetScoreButtonClicked);

    _PlayerGameButton.onClick.AddListener(PlayerWinGameClicked);
    _PlayerRoundButton.onClick.AddListener(PlayerRoundButtonClicked);
    _SetPlayerRoundsButton.onClick.AddListener(PlayerSetRoundsButtonClicked);
    _SetPlayerScoreButton.onClick.AddListener(PlayerSetScoreButtonClicked);
	
  }
	
  // Update is called once per frame
  void Update() {
	
  }

  private HomeHub GetHomeHub() {
    if (_currHub != null) {
      return _currHub;
    }
    var go = GameObject.Find("HomeHub(Clone)");
    if (go != null) {
      _currHub = go.GetComponent<HomeHub>();
      return _currHub;
    }
    return null;
  }

  private GameBase GetCurrMinigame() {
    var homeHub = GetHomeHub();
    if (homeHub != null) {
      if (homeHub.MiniGameInstance != null) {
        return homeHub.MiniGameInstance;
      }
    }
    Debug.LogWarning("CurrentMinigame is NULL, Only use these commands during a Minigame");
    return null;
  }


  // Force Game Win For Cozmo
  private void CozmoWinGameClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
      minigame.RaiseMiniGameLose();
    }
  }
  // Force Game Win For Player
  private void PlayerWinGameClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
      minigame.RaiseMiniGameWin();
    }
  }

  // Force Round Win for Cozmo
  private void CozmoRoundButtonClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
    }
  }
  // Force Round Win for Player
  private void PlayerRoundButtonClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
    }
  }

  // Set Rounds Won for Cozmo
  private void CozmoSetRoundsButtonClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
    }
  }
  // Set Rounds Won for Player
  private void PlayerSetRoundsButtonClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
    }
  }

  // Set Score for Cozmo
  private void CozmoSetScoreButtonClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
    }
  }
  // Set Score for Player
  private void PlayerSetScoreButtonClicked() {
    var minigame = GetCurrMinigame();
    if (minigame != null) {
    }
  }
}
