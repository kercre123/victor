using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace InvestorDemo {

  public class InvestorDemoGame : GameBase {

    [SerializeField]
    private InvestorDemoPanel _GamePanelPrefab;

    private InvestorDemoPanel _GamePanel;

    private InvestorDemoConfig _DemoConfig;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _DemoConfig = minigameConfig as InvestorDemoConfig;
      if (_DemoConfig == null) {
        DAS.Error(this, "Failed to load config InvestorDemoConfig!");
        return;
      }
      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {
      CurrentRobot.SetRobotVolume(1.0f);

      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<InvestorDemoPanel>();

      CurrentRobot.SetBehaviorSystem(true);

      if (_DemoConfig.UseSequence) {
        CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
        CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

        // Waiting until the investor demo buttons are registered to ObjectTagRegistry before starting
        // the sequence.
        StartCoroutine(StartSequence());
      }
      else {
        CurrentRobot.ActivateBehaviorChooser(_DemoConfig.BehaviorChooser);
      }
    }

    void Update() {
      if (_DemoConfig.UseSequence) {
        ScriptedSequences.ScriptedSequence sequence = ScriptedSequences.ScriptedSequenceManager.Instance.CurrentSequence;
        if (sequence != null) {
          List<ScriptedSequences.ScriptedSequenceNode> activeNodes = sequence.GetActiveNodes();
          if (activeNodes.Count > 0) {
            _GamePanel.SetActionText(activeNodes[0].Name);
          }
        }
      }
      else {
        _GamePanel.SetActionText(_DemoConfig.BehaviorChooser.ToString());
      }
    }

    private IEnumerator StartSequence() {
      yield return new WaitForEndOfFrame();
      ScriptedSequences.ISimpleAsyncToken token = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_DemoConfig.SequenceName);
      token.Ready(HandleSequenceComplete);
    }

    private void HandleSequenceComplete(ScriptedSequences.ISimpleAsyncToken token) {
      RaiseMiniGameWin();
    }

    protected override void CleanUpOnDestroy() {
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      CurrentRobot.CancelAllCallbacks();
      if (_DemoConfig.UseSequence) {
        ScriptedSequences.ScriptedSequence sequence = ScriptedSequences.ScriptedSequenceManager.Instance.Sequences.Find(s => s.Name == _DemoConfig.SequenceName);
        sequence.ResetSequence();
      }

      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }
    }
  }

}
