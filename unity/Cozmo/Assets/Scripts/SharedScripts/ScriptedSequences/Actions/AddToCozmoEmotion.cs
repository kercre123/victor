using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using System;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class AddToCozmoEmotion : ScriptedSequenceAction {

    [Description("The Emotion that should changed")]
    public EmotionType Emotion;
    [Description("The amount to change the emotion. The scale for emotions is [-1,1].")]
    public float Delta;
    [Description("The source of emotional change")]
    public string Source;

    public override ISimpleAsyncToken Act() {
      var robot = RobotEngineManager.Instance.CurrentRobot;

      SimpleAsyncToken token = new SimpleAsyncToken();

      if (robot == null) {
        token.Fail(new Exception("No Robot set!"));
        return token;
      }

      robot.AddToEmotion(Emotion, Delta, Source);
      token.Succeed();
      return token;
    }
  }
}
