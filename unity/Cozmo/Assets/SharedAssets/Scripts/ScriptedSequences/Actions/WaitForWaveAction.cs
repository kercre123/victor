using UnityEngine;
using System.Collections;

namespace ScriptedSequences.Actions {
  public class WaitForWaveAction : ScriptedSequenceAction {
    private SimpleAsyncToken _Token;
    private float _WaveTimeAccumulator = 0.0f;
    private float _LastWaveTime = 0.0f;

    public override ISimpleAsyncToken Act() {
      SimpleAsyncToken token = new SimpleAsyncToken();
      _Token = token;
      if (RobotEngineManager.Instance.CurrentRobot == null) {
        token.Fail();
      }
      else {
        RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
        RobotEngineManager.Instance.OnObservedMotion += HandleObservedMotion;
      }

      return token;
    }

    private void HandleObservedMotion(float x, float y) {
      if (_WaveTimeAccumulator > 0.5f) {
        _Token.Succeed();
      }

      _WaveTimeAccumulator += Mathf.Min(0.3f, Time.time - _LastWaveTime);
      _LastWaveTime = Time.time;
    }
  }
}