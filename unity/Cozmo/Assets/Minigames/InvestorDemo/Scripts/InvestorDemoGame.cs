using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace InvestorDemo {

  public class InvestorDemoGame : GameBase {
    
    private int _ActionIndex = -1;
    private bool _WaitingForCancel = false;
    private bool _WaitingForDelayed = false;

    [SerializeField]
    private InvestorDemoPanel _GamePanelPrefab;

    private InvestorDemoPanel _GamePanel;

    private InvestorDemoConfig _DemoConfig;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {

      InvestorDemoConfig demoConfig = minigameConfig as InvestorDemoConfig;
      if (demoConfig == null) {
        DAS.Error(this, "Failed to load config InvestorDemoConfig!");
        return;
      }

      _DemoConfig = demoConfig;
    }

    void Start() {
      CreateDefaultQuitButton();

      _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<InvestorDemoPanel>();
      _GamePanel.OnNextButtonPressed += HandleNextActionFromButton;
      _GamePanel.OnPrevButtonPressed += HandlePrevActionFromButton;

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

      RobotEngineManager.Instance.RobotCompletedAnimation += OnAnimationDone;

      NextAction();
    }

    private bool CurrentActionIsAnimation() {
      return !_DemoConfig.DemoActions[_ActionIndex].AnimationName.Equals("");
    }

    private void OnAnimationDone(bool success, string animationName) {
      if (_WaitingForCancel) {
        _WaitingForCancel = false;
        return;
      }
      NextAction();
    }

    private IEnumerator DelayedNextAction() {
      yield return new WaitForSeconds(0.2f);
      _WaitingForDelayed = false;
      NextAction();
    }

    private IEnumerator DelayedPrevAction() {
      yield return new WaitForSeconds(0.2f);
      _WaitingForDelayed = false;
      PrevAction();
    }


    private void NextAction() {
      _ActionIndex++;
      DoCurrentAction();
    }

    private void PrevAction() {
      _ActionIndex--;
      DoCurrentAction();
    }

    private void HandleNextActionFromButton() {
      if (_WaitingForDelayed)
        return;
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
      _WaitingForCancel = true;
      _WaitingForDelayed = true;
      StartCoroutine(DelayedNextAction());
    }

    private void HandlePrevActionFromButton() {
      if (_WaitingForDelayed)
        return;
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
      _WaitingForCancel = true;
      _WaitingForDelayed = true;
      StartCoroutine(DelayedPrevAction());
    }

    private void DoCurrentAction() {
      if (_ActionIndex >= 0 && _ActionIndex < _DemoConfig.DemoActions.Length) {
        if (CurrentActionIsAnimation()) {
          CurrentRobot.SendAnimation(_DemoConfig.DemoActions[_ActionIndex].AnimationName);
          _GamePanel.SetActionText("Playing Animation: " + _DemoConfig.DemoActions[_ActionIndex].AnimationName);

        }
        else {
          CurrentRobot.ExecuteBehavior(_DemoConfig.DemoActions[_ActionIndex].Behavior);
          _GamePanel.SetActionText("Doing: " + _DemoConfig.DemoActions[_ActionIndex].Behavior.ToString());
        }
      }
      else {
        _ActionIndex = Mathf.Clamp(_ActionIndex, 0, _DemoConfig.DemoActions.Length);
      }
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      CurrentRobot.CancelAllCallbacks();
      if (_GamePanel != null) {
        UIManager.CloseDialogImmediately(_GamePanel);
      }
    }
  }

}
