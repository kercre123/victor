using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo.MinigameWidgets;
using DG.Tweening;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  protected void RaiseMiniGameQuit() {
    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }
  }

  public delegate void MiniGameWinHandler();

  public event MiniGameWinHandler OnMiniGameWin;

  public void RaiseMiniGameWin() {
    if (OnMiniGameWin != null) {
      OnMiniGameWin();
    }
  }

  public delegate void MiniGameLoseHandler();

  public event MiniGameWinHandler OnMiniGameLose;

  public void RaiseMiniGameLose() {
    if (OnMiniGameLose != null) {
      OnMiniGameLose();
    }
  }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  protected SharedMinigameView _SharedMinigameViewInstance;

  /// <summary>
  /// Order of operations:
  /// Call InitializeMinigameObjects();
  /// Call LoadMinigameConfig();
  /// Create the minigame view and assign to _SharedMinigameViewInstance.
  /// Call InitializeMinigameView();
  /// </summary>
  public void InitializeMinigame(MinigameConfigBase minigameConfigData) {
    Initialize(minigameConfigData);

    GameObject minigameViewObj = UIManager.CreateUIElement(UIPrefabHolder.Instance.SharedMinigameViewPrefab.gameObject);
    _SharedMinigameViewInstance = minigameViewObj.GetComponent<SharedMinigameView>();

    // Populate the view before opening it so that animations play correctly
    InitializeMinigameView(_SharedMinigameViewInstance);
    _SharedMinigameViewInstance.OpenView();
  }

  /// <summary>
  /// Order of operations:
  /// Call InitializeMinigameObjects();
  /// Call InitializeMinigameData();
  /// Create the minigame view and assign to _SharedMinigameViewInstance.
  /// Call InitializeMinigameView();
  /// </summary>
  protected abstract void Initialize(MinigameConfigBase minigameConfigData);

  /// <summary>
  /// Order of operations:
  /// Call InitializeMinigameObjects();
  /// Call InitializeMinigameData();
  /// Create the minigame view and assign to _SharedMinigameViewInstance.
  /// Call InitializeMinigameView();
  /// </summary>
  protected virtual void InitializeMinigameView(SharedMinigameView minigameView) {
    // Override and call create stuff here
    CreateDefaultQuitButton(minigameView);
  }

  public void OnDestroy() {
    CleanUpOnDestroy();
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  protected virtual void PauseGame() {
    _SharedMinigameViewInstance.DisableInteractivity();
  }

  protected virtual void ResumeGame() {
    _SharedMinigameViewInstance.EnableInteractivity();
  }

  public void CloseMinigame() {
    _SharedMinigameViewInstance.ViewCloseAnimationFinished += HandleMinigameViewCloseFinished;
    _SharedMinigameViewInstance.CloseView();
    _SharedMinigameViewInstance = null;
  }

  public void CloseMinigameImmediately() {
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    HandleMinigameViewCloseFinished();
  }

  private void HandleMinigameViewCloseFinished() {
    Destroy(gameObject);
  }

  #region Default Quit button

  protected void CreateDefaultQuitButton(SharedMinigameView minigameView) {
    minigameView.CreateQuitButton();
    minigameView.QuitMiniGameViewOpened += HandleQuitViewOpened;
    minigameView.QuitMiniGameViewClosed += HandleQuitViewClosed;
    minigameView.QuitMiniGameConfirmed += HandleQuitConfirmed;
  }

  private void HandleQuitViewOpened() {
    PauseGame();
  }

  private void HandleQuitViewClosed() {
    ResumeGame();
  }

  private void HandleQuitConfirmed() {
    RaiseMiniGameQuit();
  }

  #endregion

  #region Default Stamina Bar

  private void CreateDefaultStaminaBar() {
  }

  #endregion
}