using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WhackAMole {
  // TODO : In this state, alternate between targets on random intervals with increasing frequency.
  // Returns to normal chase if mole state is not equal to BOTH
  // Plays Frustrated Animation, deactivates both, and sets state to Idle if you remain
  // in this state for too long. Potentially have Cozmo hide or cower until mole state is not
  // equal to BOTH.
  public class WhackAMolePanic : State {
    private WhackAMoleGame _WhackAMoleGame;
    private float _PanicTimeout;
    private float _PanicStartTimestamp = -1;
    private float _PanicInterval;
    private float _PanicIntervalTimestamp = -1;

    public override void Enter() {
      base.Enter();
      _WhackAMoleGame = (_StateMachine.GetGame() as WhackAMoleGame);
      _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      _CurrentRobot.SetLiftHeight(1.0f);
      _PanicTimeout = _WhackAMoleGame.MaxPanicTime;
      _PanicInterval = _WhackAMoleGame.MaxPanicInterval;
      _PanicStartTimestamp = Time.time;
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
      _CurrentRobot.PickupObject(_WhackAMoleGame.CurrentTarget,true,false,false,0,PanicTargetSwitch);
    }

    public override void Update() {
      base.Update();
      if (_PanicIntervalTimestamp == -1) {
        _PanicIntervalTimestamp = Time.time;
      }

      // Check timestamps for Panic Target Switching and Timeout
      if (Time.time - _PanicIntervalTimestamp > _PanicInterval) {
        PanicTargetSwitch(true);
      }
      if (Time.time - _PanicStartTimestamp > _PanicTimeout) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorFail, HandleAnimationDone));
      }
    }

    // Reset the interval with decay as Cozmo panics more.
    // TODO: Factor this into the Mood System?
    private void PanicTargetSwitch(bool success) {
      _PanicIntervalTimestamp = -1;
      _PanicInterval = _PanicInterval - (_PanicInterval * Random.Range(_WhackAMoleGame.PanicDecayMin, _WhackAMoleGame.PanicDecayMax));
      int curr;
      if (_WhackAMoleGame.CurrentTarget != null) {
        curr = _WhackAMoleGame.CurrentTarget.ID;
      }
      else {
        curr = -1;
      }
      foreach (KeyValuePair<int, LightCube> kVp in _WhackAMoleGame.ActivatedCubes) {
        if (kVp.Key != curr) {
          _WhackAMoleGame.CurrentTarget = kVp.Value;
          _CurrentRobot.PickupObject(_WhackAMoleGame.CurrentTarget,true,false,false,0,PanicTargetSwitch);
          return;
        }
      }
    }

    void HandleAnimationDone(bool success) {
      _WhackAMoleGame.SetUpCubes();
    }
    void HandleMoleStateChange(WhackAMoleGame.MoleState state) {
      if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.NONE) {
        // A cube has been tapped, start chase. If more than one cube is
        // active, Chase will handle moving to Panic.
        _StateMachine.SetNextState(new WhackAMoleConfusion());
      }
      else if (_WhackAMoleGame.CubeState == WhackAMoleGame.MoleState.SINGLE) {
        _StateMachine.SetNextState(new WhackAMoleChase());
      }
    }

    public override void Exit() {
      base.Exit();
      _WhackAMoleGame.MoleStateChanged -= HandleMoleStateChange;
    }
  }
}
