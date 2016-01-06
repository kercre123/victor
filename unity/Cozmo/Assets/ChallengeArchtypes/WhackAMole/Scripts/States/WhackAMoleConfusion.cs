using UnityEngine;
using System.Collections;

namespace WhackAMole {
  // TODO : In this state, looks around confused, trying to find cube.
  // If mole state is not NONE, play surprise animation then move to Chase
  // If mole state is NONE for too long, play disappointed animation
  // then return to Idle state. 
  public class WhackAMoleConfusion : State {
    private WhackAMoleGame _WhackAMoleGame;
    private float _ConfusionTimeout;
    private float _ConfusionStartTimestamp;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _ConfusionTimeout = _WhackAMoleGame.MaxConfuseTime;
      _ConfusionStartTimestamp = Time.time;
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }

    public override void Update() {
      base.Update();
      if (Time.time - _ConfusionStartTimestamp > _ConfusionTimeout) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kHeadDown, HandleAnimationDone));
      }
    }

    void HandleAnimationDone(bool success) {
      _StateMachine.SetNextState(new WhackAMoleIdle());
    }

    void HandleMoleStateChange(WhackAMoleGame.MoleState state) {
      Debug.Log(string.Format("Confusion - Mole State Changed to {0}", state));
      if (_WhackAMoleGame.CubeState != WhackAMoleGame.MoleState.NONE) {
        // A cube has been tapped, start chase. If more than one cube is
        // active, Chase will handle moving to Panic.
        _StateMachine.SetNextState(new WhackAMoleChase());
      }
    }

    public override void Exit() {
      base.Exit();
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
