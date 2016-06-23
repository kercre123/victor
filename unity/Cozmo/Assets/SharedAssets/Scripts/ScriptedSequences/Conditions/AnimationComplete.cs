using System;
using Anki.Cozmo;

namespace ScriptedSequences.Conditions {
  public class AnimationComplete : AbstractCozmoCondition {

    public bool AnyAnimation;
    public string AnimationName;

    protected override void EnableChangedAndRobotConnected(bool enabled) {
      if (enabled) {
        RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.ExternalInterface.RobotCompletedAction), HandleActionComplete);
      }
      else {
        RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.ExternalInterface.RobotCompletedAction), HandleActionComplete);
      }
    }

    private void HandleActionComplete(object messageObject) {
      Anki.Cozmo.ExternalInterface.RobotCompletedAction message = (Anki.Cozmo.ExternalInterface.RobotCompletedAction)messageObject;
      RobotActionType actionType = message.actionType;
      bool success = message.result == ActionResult.SUCCESS;

      if (actionType == RobotActionType.PLAY_ANIMATION) {
        if (message.completionInfo.GetTag() == ActionCompletedUnion.Tag.animationCompleted) {
          // Reset cozmo's face
          string animationCompleted = message.completionInfo.animationCompleted.animationName;
          HandleAnimationComplete(success, animationCompleted);
        }
        else {
          DAS.Warn("RobotEngineManager.ReceivedSpecificMessage(G2U.RobotCompletedAction message)",
            string.Format("Message is of type RobotActionType.PLAY_ANIMATION but message.completionInfo has tag {0} instead of ActionCompletedUnion.Tag.animationCompleted! idTag={1} result={2}",
              message.completionInfo.GetTag(), message.idTag, message.result));
        }
      }
    }

    void HandleAnimationComplete(bool success, string animationName) {
      if (AnyAnimation || AnimationName == animationName) {
        IsMet = true;
      }
    }
  }
}

