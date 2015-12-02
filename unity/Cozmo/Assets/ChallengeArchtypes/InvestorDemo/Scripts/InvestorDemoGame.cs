using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace InvestorDemo {

  public class InvestorDemoGame : GameBase {

    [SerializeField]
    private InvestorDemoPanel _GamePanelPrefab;

    private InvestorDemoPanel _GamePanel;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
      InvestorDemoConfig demoConfig = minigameConfig as InvestorDemoConfig;
      if (demoConfig == null) {
        DAS.Error(this, "Failed to load config InvestorDemoConfig!");
        return;
      }



    }

    void Start() {
      CreateDefaultQuitButton();

      CurrentRobot.SetRobotVolume(1.0f);

      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<InvestorDemoPanel>();

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

      // Waiting until the investor demo buttons are registered to ObjectTagRegistry before starting
      // the sequence.
      StartCoroutine(StartSequence());

    }

    void Update() {
      ScriptedSequences.ScriptedSequence sequence = ScriptedSequences.ScriptedSequenceManager.Instance.CurrentSequence;
      if (sequence != null) {
        List<ScriptedSequences.ScriptedSequenceNode> activeNodes = sequence.GetActiveNodes();
        if (activeNodes.Count > 0) {
          _GamePanel.SetActionText(activeNodes[0].Name);
        }
      }
    }

    private IEnumerator StartSequence() {
      yield return new WaitForEndOfFrame();
      ScriptedSequences.ISimpleAsyncToken token = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence("InvestorScene1");
      token.Ready(HandleSequenceComplete);
    }

    private void HandleSequenceComplete(ScriptedSequences.ISimpleAsyncToken token) {
      RaiseMiniGameWin();
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      CurrentRobot.CancelAllCallbacks();
      ScriptedSequences.ScriptedSequence sequence = ScriptedSequences.ScriptedSequenceManager.Instance.Sequences.Find(s => s.Name == "InvestorScene1");
      sequence.ResetSequence();
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }
    }
  }

}
