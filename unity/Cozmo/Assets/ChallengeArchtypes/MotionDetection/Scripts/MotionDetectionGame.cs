using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace MotionDetection {

  public class MotionDetectionGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private MotionDetectionConfig _Config;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as MotionDetectionConfig ?? new MotionDetectionConfig();
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("DetectMotionStateMachine", _StateMachine);
      _StateMachine.SetNextState(new RecognizeMotionState(_Config.TimeAllowedBetweenWaves, _Config.TotalWaveTime));
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);

      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}
