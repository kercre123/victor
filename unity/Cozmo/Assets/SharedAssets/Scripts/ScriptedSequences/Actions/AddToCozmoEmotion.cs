using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using System;

namespace ScriptedSequences.Actions {
  public class AddToCozmoEmotion : ScriptedSequenceAction {
    public EmotionType Emotion;
    public float Delta;

    public override ISimpleAsyncToken Act() {
      var robot = RobotEngineManager.Instance.CurrentRobot;

      SimpleAsyncToken token = new SimpleAsyncToken();

      if (robot == null) {
        token.Fail(new Exception("No Robot set!"));
        return token;
      }

      robot.AddToEmotion(Emotion, Delta);
      token.Succeed();
      return token;
    }
  }
}
