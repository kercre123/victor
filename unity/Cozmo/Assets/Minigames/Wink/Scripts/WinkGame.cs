using UnityEngine;
using System.Collections;

namespace Wink {
  public class WinkGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private bool _WinkWaveCompleted = false;
    private float _WinkWaveAccumulator = 0.0f;

    public enum WinkStatus {
      Left,
      Right,
      Neutral
    }

    private WinkStatus _WinkStatus = WinkStatus.Neutral;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfigData) {
      
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("WinkGameStateMachine", _StateMachine);
      _StateMachine.SetNextState(new WinkState());
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
      CheckWaveComplete();
    }

    public void PickNewWinkSide() {
      if (Random.Range(0.0f, 1.0f) < 0.5f) {
        _WinkStatus = WinkStatus.Left;
      }
      else {
        _WinkStatus = WinkStatus.Right;
      }
      SetProceduralFace();
    }

    public WinkStatus GetWinkStatus() {
      return _WinkStatus;
    }

    public bool GetWinkCompleted() {
      return _WinkWaveCompleted;
    }

    public void DoneWink() {
      _WinkWaveCompleted = false;
      _WinkStatus = WinkStatus.Neutral;
      SetProceduralFace();
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }

    private void CheckWaveComplete() {
      if ((WaveLeft() && _WinkStatus == WinkStatus.Left) || (WaveRight() && _WinkStatus == WinkStatus.Right)) {
        _WinkWaveAccumulator += Time.deltaTime;
      }
      else {
        _WinkWaveAccumulator -= Time.deltaTime;
      }
    }

    private bool WaveLeft() {
      return false;
    }

    private bool WaveRight() {
      return false;
    }

    private void SetProceduralFace() {
      switch (_WinkStatus) {
      case WinkStatus.Left:
        break;
      case WinkStatus.Right:
        break;
      case WinkStatus.Neutral:
        CurrentRobot.SendAnimation(AnimationName.kDefaultFace);
        break;
      }
    }
  }
}
