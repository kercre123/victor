using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using System;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class EnterCozmoBehavior : ScriptedSequenceAction {

    [Description("The Behavior State you want Cozmo to be in")]
    public BehaviorType Behavior;

    public override ISimpleAsyncToken Act() {
      var robot = RobotEngineManager.Instance.CurrentRobot;

      SimpleAsyncToken token = new SimpleAsyncToken();

      if (robot == null) {
        token.Fail(new Exception("No Robot set!"));
        return token;
      }

      robot.ExecuteBehavior(Behavior);
      token.Succeed();
      return token;
    }
  }
}
