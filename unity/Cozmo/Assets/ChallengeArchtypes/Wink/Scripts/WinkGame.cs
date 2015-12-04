using UnityEngine;
using System.Collections;

namespace Wink {
  public class WinkGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private bool _WinkWaveCompleted = false;
    private float _WinkWaveAccumulator = 0.0f;
    private float _LastWaveMessageTime = 0.0f;
    private int _WaveSuccessCount = 0;

    public enum WinkStatus {
      Left,
      Right,
      Neutral
    }

    private WinkStatus _WinkStatus = WinkStatus.Neutral;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfigData) {
      
    }

    public void WaveSuccess() {
      _WaveSuccessCount++;
      if (_WaveSuccessCount > 3) {
        RaiseMiniGameWin();
      }
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("WinkGameStateMachine", _StateMachine);
      _StateMachine.SetNextState(new WinkState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);

      RobotEngineManager.Instance.OnObservedMotion += OnMotionDetected;

      OpenMinigameView();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
      _WinkWaveAccumulator = Mathf.Max(0.0f, _WinkWaveAccumulator - Time.deltaTime * 0.25f);
      if (_WinkWaveAccumulator > 0.6f) {
        _WinkWaveCompleted = true;
      }
    }

    public void PickNewWinkSide() {
      if (Random.Range(0.0f, 1.0f) < 0.5f) {
        _WinkStatus = WinkStatus.Left;
      }
      else {
        _WinkStatus = WinkStatus.Right;
      }
      SetProceduralFace();
      _WinkWaveAccumulator = 0.0f;
      _WinkWaveCompleted = false;
    }

    public WinkStatus GetWinkStatus() {
      return _WinkStatus;
    }

    public bool GetWinkCompleted() {
      return _WinkWaveCompleted;
    }

    public void DoneWink() {
      _WinkWaveAccumulator = 0.0f;
      _WinkWaveCompleted = false;
      _WinkStatus = WinkStatus.Neutral;
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.OnObservedMotion -= OnMotionDetected;
    }

    private void OnMotionDetected(Vector2 pos) {

      if (_WinkStatus == WinkStatus.Neutral) {
        return;
      }

      if (_WinkStatus == WinkStatus.Left && pos.x < 0.0f) {
        return;
      }

      if (_WinkStatus == WinkStatus.Right && pos.x > 0.0f) {
        return;
      }
      
      float motionDetectedTime = Mathf.Clamp(Time.time - _LastWaveMessageTime, 0.0f, 0.3f);
      _WinkWaveAccumulator += motionDetectedTime;

      _LastWaveMessageTime = Time.time;
    }

    private void SetProceduralFace() {
      switch (_WinkStatus) {
      case WinkStatus.Left:
        CurrentRobot.SendAnimation(AnimationName.kFaceWinkL);
        break;
      case WinkStatus.Right:
        CurrentRobot.SendAnimation(AnimationName.kFaceWinkR);
        break;
      case WinkStatus.Neutral:
        break;
      }
    }
  }
}
