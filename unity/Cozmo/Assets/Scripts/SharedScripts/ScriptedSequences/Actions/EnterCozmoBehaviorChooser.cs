using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using System;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class EnterCozmoBehaviorChooser : ScriptedSequenceAction {

    [Description("The Behavior Chooser you want Cozmo to be in")]
    public HighLevelActivity Activity;

    public override ISimpleAsyncToken Act() {
      var robot = RobotEngineManager.Instance.CurrentRobot;

      SimpleAsyncToken token = new SimpleAsyncToken();

      if (robot == null) {
        token.Fail(new Exception("No Robot set!"));
        return token;
      }

      robot.ActivateHighLevelActivity(Activity);
      token.Succeed();
      return token;
    }
  }
}
