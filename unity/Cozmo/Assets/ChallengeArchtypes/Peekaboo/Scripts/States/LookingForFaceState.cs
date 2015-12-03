using UnityEngine;
using System.Collections;

namespace Peekaboo {
  // Default : Wander about, trying to find a face, goes to FoundFaceState if face is detected
  public class LookingForFaceState : State {
    
    PeekGame _GameInstance;

    public override void Enter() {
      base.Enter();

      _GameInstance = _StateMachine.GetGame() as PeekGame;

      _CurrentRobot.SetHeadAngle(0);
      _CurrentRobot.SetLiftHeight(0);
      // Play Confused Animation as you enter this state, then begin
      // to wander aimlessly looking for a new face.
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);

    }

    public override void Update() {
      base.Update();
      // Wander aimlessly
      // If a face has been found, enter FoundFaceState
      if (_CurrentRobot.Faces.Count > 0) {
        // We found a face!
        FindFace();
      }
    }

    public void FindFace() {
      _StateMachine.SetNextState(new FoundFaceState());
    }
  }

}
