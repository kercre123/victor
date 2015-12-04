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

  protected void RaiseMiniGameLose() {
    if (OnMiniGameLose != null) {
      OnMiniGameLose();
    }
  }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private QuitMinigameButton _QuitButtonInstance;

  private List<IMinigameWidget> _ActiveWidgets = new List<IMinigameWidget>();

  public abstract void LoadMinigameConfig(MinigameConfigBase minigameConfigData);

  private Sequence _MinigameViewSequence;

  public void OnDestroy() {
    if (_MinigameViewSequence != null) {
      _MinigameViewSequence.Kill();
    }
    CleanUpOnDestroy();
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  protected virtual void PauseGame() {
    foreach (IMinigameWidget widget in _ActiveWidgets) {
      widget.DisableInteractivity();
    }
  }

  protected virtual void ResumeGame() {
    foreach (IMinigameWidget widget in _ActiveWidgets) {
      widget.EnableInteractivity();
    }
  }

  protected void OpenMinigameView() {
    // TODO: Play animations from subclasses?
  }

  public void CloseMinigameView() {
    // TODO: Play an animation on the quit button if it exists
    // TODO: Play an animation on the other UI if it exists
    // TODO: Join sequences
    if (_MinigameViewSequence != null) {
      _MinigameViewSequence.Kill();
    }
    _MinigameViewSequence = DOTween.Sequence();
    Sequence close;
    foreach (IMinigameWidget widget in _ActiveWidgets) {
      close = widget.CloseAnimationSequence();
      if (close != null) {
        _MinigameViewSequence.Append(close);
      }
    }
    _MinigameViewSequence.AppendCallback(HandleMinigameViewCloseAnimationFinished);
  }

  private void HandleMinigameViewCloseAnimationFinished() {
    CloseMinigameViewImmediately();
  }

  public void CloseMinigameViewImmediately() {
    foreach (IMinigameWidget widget in _ActiveWidgets) {
      widget.DestroyWidgetImmediately();
    }
    _ActiveWidgets.Clear();
    Destroy(gameObject);
  }

  #region Default Quit button

  protected void CreateDefaultQuitButton() {
    GameObject newButton = UIManager.CreateUIElement(UIPrefabHolder.Instance.DefaultQuitGameButtonPrefab);
    // TODO: use ankibutton
    _QuitButtonInstance = newButton.GetComponent<QuitMinigameButton>();

    _QuitButtonInstance.QuitViewOpened += HandleQuitViewOpened;
    _QuitButtonInstance.QuitViewClosed += HandleQuitViewClosed;
    _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;

    _ActiveWidgets.Add(_QuitButtonInstance);
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