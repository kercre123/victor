using UnityEngine;
using System.Collections;

namespace FaceEnrollment {
  public class FaceEditDoneState : State {

    private int _FaceID;
    private string _OldNameForFace;
    private string _NewNameForFace;

    private FaceEnrollmentGame _FaceEnrollmentGame;

    public override void Pause(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
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

      // Send rename and wait for confirmation
      _CurrentRobot.OnEnrolledFaceRenamed += HandleRobotRenamedEnrolledFace;
      _CurrentRobot.UpdateEnrolledFaceByID(_FaceID, _OldNameForFace, _NewNameForFace);
    }

    public override void Exit() {
      base.Exit();
      _FaceEnrollmentGame.ShowDoneShelf = true;
    }

    private void HandleRobotRenamedEnrolledFace(int faceId, string faceName) {
      // Rename confirmed: now go back to face slide view (with updated name)
      _CurrentRobot.OnEnrolledFaceRenamed -= HandleRobotRenamedEnrolledFace;
      _FaceEnrollmentGame.SharedMinigameView.HideBackButton();
      _StateMachine.SetNextState(new FaceSlideState());
    }
  }

}
