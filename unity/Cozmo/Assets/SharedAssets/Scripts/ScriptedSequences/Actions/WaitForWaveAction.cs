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
        token.Fail(new System.Exception("Current Robot Is Null"));
      }
      else {
        RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotObservedMotion>(HandleObservedMotion);
      }

      return token;
    }

    private void HandleObservedMotion(object messageObject) {

      /*
       *  here's how to compute to normalized screen space.
       * 
       * if (OnObservedMotion != null) {
            var resolution = ImageResolutionTable.GetDimensions(ImageResolution.CVGA);
            // Normalize our position to [-1,1], [-1,1]
            OnObservedMotion(new Vector2(message.img_x * 2.0f / (float)resolution.Width, message.img_y * 2.0f / (float)resolution.Height));
         }
       * */

      if (_WaveTimeAccumulator > 0.5f) {
        _Token.Succeed();
      }

      _WaveTimeAccumulator += Mathf.Min(0.3f, Time.time - _LastWaveTime);
      _LastWaveTime = Time.time;
    }
  }
}