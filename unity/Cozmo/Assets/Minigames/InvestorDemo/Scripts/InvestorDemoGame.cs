using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace InvestorDemo {

  public class InvestorDemoGame : GameBase {
    
    private int _ActionIndex = -1;

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

      NextAction();
    }

    private void OnAnimationDone(bool success) {
      // not calling NextAction here directly because we need to wait a frame for the
      // RobotCallbacks to be updated correctly.
      StartCoroutine(DelayedNextAction());
    }

    private IEnumerator DelayedNextAction() {
      yield return new WaitForEndOfFrame();
      NextAction();
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
      CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
      CurrentRobot.CancelCallback(OnAnimationDone);
      NextAction();
    }

    private void HandlePrevActionFromButton() {
      CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
      CurrentRobot.CancelCallback(OnAnimationDone);
      PrevAction();
    }

    private void DoCurrentAction() {
      if (_ActionIndex >= 0 && _ActionIndex < _DemoConfig.DemoActions.Length) {
        if (!_DemoConfig.DemoActions[_ActionIndex].AnimationName.Equals("")) {
          CurrentRobot.SendAnimation(_DemoConfig.DemoActions[_ActionIndex].AnimationName, OnAnimationDone);
          CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
          _GamePanel.SetActionText("Playing Animation: " + _DemoConfig.DemoActions[_ActionIndex].AnimationName);

        }
        else {
          CurrentRobot.ExecuteBehavior(_DemoConfig.DemoActions[_ActionIndex].Behavior);
          _GamePanel.SetActionText("Doing: " + _DemoConfig.DemoActions[_ActionIndex].Behavior.ToString());
        }
      }
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
      CurrentRobot.CancelAllCallbacks();
      if (_GamePanel != null) {
        UIManager.CloseDialogImmediately(_GamePanel);
      }
    }
  }

}
