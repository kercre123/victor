using UnityEngine;
using System.Collections;
namespace FaceEnrollment {
  public class FaceEditDoneState : State {

    private int _FaceID;
    private string _OldNameForFace;
    private string _NewNameForFace;

    private FaceEnrollmentGame _FaceEnrollmentGame;

    public override void Pause(State.PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    public void Initialize(int faceID, string oldNameForFace, string newFaceName) {
      _FaceID = faceID;
      _NewNameForFace = newFaceName;
      _OldNameForFace = oldNameForFace;
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;

      _CurrentRobot.OnEnrolledFaceRenamed += HandleRobotRenamedEnrolledFace;
      _CurrentRobot.UpdateEnrolledFaceByID(_FaceID, _OldNameForFace, _NewNameForFace);
    }

    public override void Exit() {
      base.Exit();
      _FaceEnrollmentGame.ShowDoneShelf = true;
    }

    private void HandleRobotRenamedEnrolledFace(int faceId, string faceName) {
      _CurrentRobot.OnEnrolledFaceRenamed -= HandleRobotRenamedEnrolledFace;
      _FaceEnrollmentGame.SharedMinigameView.HideBackButton();
      _CurrentRobot.SayTextWithEvent(faceName, Anki.Cozmo.AnimationTrigger.MeetCozmoRenameFaceSayName, Anki.Cozmo.SayTextIntent.Name_Normal, callback: (success) => {
        _StateMachine.SetNextState(new FaceSlideState());
      });
    }
  }

}
