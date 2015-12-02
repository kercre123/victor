using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using System;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class EnterCozmoBehaviorChooser : ScriptedSequenceAction {

    [Description("The Behavior Chooser you want Cozmo to be in")]
    public BehaviorChooserType BehaviorChooser;

    public override ISimpleAsyncToken Act() {
      var robot = RobotEngineManager.Instance.CurrentRobot;

      SimpleAsyncToken token = new SimpleAsyncToken();

      if (robot == null) {
        token.Fail(new Exception("No Robot set!"));
        return token;
      }

      robot.ActivateBehaviorChooser(BehaviorChooser);
      token.Succeed();
      return token;
    }
  }
}
