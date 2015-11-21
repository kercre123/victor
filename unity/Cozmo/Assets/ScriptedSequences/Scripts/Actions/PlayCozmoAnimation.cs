using System;

namespace ScriptedSequences.Actions {
  public class PlayCozmoAnimation : ScriptedSequenceAction {

    public string AnimationName;

    public bool WaitToEnd = true;
  
    public override ISimpleAsyncToken Act() {
      var robot = RobotEngineManager.Instance.CurrentRobot;

      SimpleAsyncToken token = new SimpleAsyncToken();

      if (robot == null) {
        token.Fail(new Exception("No Robot set!"));
        return token;
      }

      if (WaitToEnd) {
        robot.SendAnimation(AnimationName, (s) => { 
          // Do we want to fail the action if playing the animation failed?
          token.Succeed();
        });
      }
      else {
        robot.SendAnimation(AnimationName);
        token.Succeed();
      }
      return token;
    }
  }
}

