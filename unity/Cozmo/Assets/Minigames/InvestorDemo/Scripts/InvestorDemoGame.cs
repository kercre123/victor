using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace InvestorDemo {

  public class InvestorDemoGame : GameBase {

    private List<DemoAction> _DemoActions = new List<DemoAction>();
    private int _ActionIndex = -1;

    [SerializeField]
    private UnityEngine.UI.Text _CurrentActionText;

    [SerializeField]
    private Button _NextButton;

    [SerializeField]
    private Button _PrevButton;

    void Start() {
      CreateDefaultQuitButton();

      _DemoActions.Add(new DemoAction(AnimationName.kInvestorDemo1, Anki.Cozmo.BehaviorType.NoneBehavior));
      _DemoActions.Add(new DemoAction(AnimationName.kInvestorDemo2, Anki.Cozmo.BehaviorType.NoneBehavior));
      _DemoActions.Add(new DemoAction("", Anki.Cozmo.BehaviorType.LookAround));

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
      if (_ActionIndex > 0 && _ActionIndex < _DemoActions.Count) {
        if (!_DemoActions[_ActionIndex].AnimationName.Equals("")) {
          CurrentRobot.SendAnimation(_DemoActions[_ActionIndex].AnimationName, OnAnimationDone);
          CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
          _CurrentActionText.text = "Playing Animation: " + _DemoActions[_ActionIndex].AnimationName;

        }
        else {
          CurrentRobot.ExecuteBehavior(_DemoActions[_ActionIndex].Behavior);
          _CurrentActionText.text = "Doing: " + _DemoActions[_ActionIndex].Behavior.ToString();
        }
      }
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }

}
