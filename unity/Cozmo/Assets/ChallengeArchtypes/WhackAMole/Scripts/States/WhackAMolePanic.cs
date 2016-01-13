using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WhackAMole {
  // In this state, alternate between targets on random intervals with increasing frequency.
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
      _PanicTimeout = _WhackAMoleGame.MaxPanicTime;
      _PanicInterval = _WhackAMoleGame.MaxPanicInterval;
      _PanicStartTimestamp = Time.time;
      PanicTargetSwitch(true);
      _WhackAMoleGame.MoleStateChanged += HandleMoleStateChange;
    }
      
    public override void Update() {
      base.Update();
      if (_PanicIntervalTimestamp == -1) {
        _PanicIntervalTimestamp = Time.time;
      }

      if (Time.time - _PanicStartTimestamp > _PanicTimeout) {
        _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorFail, HandleAnimationDone));
        return;
      }
      // Check timestamps for Panic Target Switching and Timeout
      if (Time.time - _PanicIntervalTimestamp > _PanicInterval) {
        PanicTargetSwitch(true);
      }
    }

    // Reset the interval with decay as Cozmo panics more.
    private void PanicTargetSwitch(bool success) {
      if (_WhackAMoleGame.CubeState != WhackAMoleGame.MoleState.BOTH) {
        return;
      }
      _WhackAMoleGame.FixCozmoAngles();
      _PanicIntervalTimestamp = -1;
      _PanicInterval = _PanicInterval - (Random.Range(_WhackAMoleGame.PanicDecayMin, _WhackAMoleGame.PanicDecayMax));
      if (_PanicInterval <= _WhackAMoleGame.PanicDecayMin) {
        _PanicInterval = _WhackAMoleGame.PanicDecayMin;
      }
      KeyValuePair<int,int> curr;
      if (_WhackAMoleGame.CurrentTargetKvP.Equals(null)) {
        curr = _WhackAMoleGame.CurrentTargetKvP;
      }
      else {
        curr = new KeyValuePair<int,int>(-1,-1);
      }
      // Grab a different face to chase after. TODO: Make this more random
      foreach (KeyValuePair<int, int> kVp in _WhackAMoleGame.ActivatedFaces) {
        if (!kVp.Equals(curr)) {
          _WhackAMoleGame.CurrentTargetKvP = kVp;
          Debug.Log(string.Format("Panic - Now Target Cube {0}", kVp.Key));
          _CurrentRobot.AlignWithObject(_CurrentRobot.LightCubes[kVp.Key], 150f , null,true, 
            _WhackAMoleGame.GetRelativeRad(kVp), Anki.Cozmo.QueueActionPosition.NOW_AND_CLEAR_REMAINING);
          return;
        }
      }
    }

    void HandleAnimationDone(bool success) {
      Debug.Log("Panic Animation Done, Deactivate all Cubes and return to idle.");
      _WhackAMoleGame.InitializeButtons();
      _StateMachine.SetNextState(new WhackAMoleIdle());
    }

    void HandleMoleStateChange(WhackAMoleGame.MoleState state) {
      Debug.Log(string.Format("Panic - Mole State Changed to {0}", state));
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
