using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace InvestorDemo {

  public class InvestorDemoGame : GameBase {
    
    private int _ActionIndex = -1;

    [SerializeField]
    private UnityEngine.UI.Text _CurrentActionText;

    [SerializeField]
    private Button _NextButton;

    [SerializeField]
    private Button _PrevButton;

    private InvestorDemoConfig _DemoConfig;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {

      InvestorDemoConfig demoConfig = minigameConfig as InvestorDemoConfig;
      if (demoConfig == null) {
        DAS.Error(this, "Failed to load config InvestorDemoConfig!");
        return;
      }

      _DemoConfig = demoConfig;

      ConfigLoaded();

    }

    void Start() {
      CreateDefaultQuitButton();
    }

    private void ConfigLoaded() {
      NextAction();

      _NextButton.onClick.AddListener(NextAction);
      _NextButton.onClick.AddListener(PrevAction);
    }

    private void OnAnimationDone(bool success) {
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

    private void DoCurrentAction() {
      if (_ActionIndex > 0 && _ActionIndex < _DemoConfig.DemoActions.Length) {
        if (!_DemoConfig.DemoActions[_ActionIndex].AnimationName.Equals("")) {
          CurrentRobot.SendAnimation(_DemoConfig.DemoActions[_ActionIndex].AnimationName, OnAnimationDone);
          CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
          _CurrentActionText.text = "Playing Animation: " + _DemoConfig.DemoActions[_ActionIndex].AnimationName;

        }
        else {
          CurrentRobot.ExecuteBehavior(_DemoConfig.DemoActions[_ActionIndex].Behavior);
          _CurrentActionText.text = "Doing: " + _DemoConfig.DemoActions[_ActionIndex].Behavior.ToString();
        }
      }
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }

}
