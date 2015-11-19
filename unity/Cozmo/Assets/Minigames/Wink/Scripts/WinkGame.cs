using UnityEngine;
using System.Collections;

namespace Wink {
  public class WinkGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public enum WinkStatus {
      Left,
      Right,
      Neutral
    }

    private WinkStatus _WinkStatus = WinkStatus.Neutral;

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("WinkGameStateMachine", _StateMachine);
      _StateMachine.SetNextState(new WinkState());
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    public void PickNewWinkSide() {
      if (Random.Range(0.0f, 1.0f) < 0.5f) {
        _WinkStatus = WinkStatus.Left;
      }
      else {
        _WinkStatus = WinkStatus.Right;
      }
    }

    public WinkStatus GetWinkStatus() {
      return _WinkStatus;
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }
}
