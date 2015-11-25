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

      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<InvestorDemoPanel>();
      _GamePanel.OnNextButtonPressed += HandleNextActionFromButton;

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);

    }

    private void HandleNextActionFromButton() {

    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      CurrentRobot.CancelAllCallbacks();
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }
    }
  }

}
