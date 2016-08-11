using Cozmo.UI;
using UnityEngine;
using Anki.Cozmo;

namespace Onboarding {

  public class PlayAnimStage : OnboardingBaseStage {

    [SerializeField]
    private SerializableAnimationTrigger _Animation;

    [SerializeField]
    private float _LookForFacesMaxTime_Sec = 8.0f;

    private float _EndLookAtFacesTime_Sec = -1.0f;
    private bool _HasObservedFace = false;
    private bool _CompletedInitialAnimation = false;

    private IRobot _CurrentRobot;
    public override void Start() {
      base.Start();
      _CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding);
      _CurrentRobot.SendAnimationTrigger(_Animation.Value, HandleEndAnimationComplete);
    }

    public override void OnDestroy() {
      base.OnDestroy();
      _CurrentRobot.ExecuteBehavior(BehaviorType.NoneBehavior);
    }

    public override void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }

    private void HandleEndAnimationComplete(bool success) {
      _CompletedInitialAnimation = true;
      if (_LookForFacesMaxTime_Sec > 0) {
        // this should really have a 'goal' of finding faces not behavior when that gets in.
        _EndLookAtFacesTime_Sec = Time.time + _LookForFacesMaxTime_Sec;
        _CurrentRobot.ExecuteBehavior(BehaviorType.FindFaces);
      }
      else {
        OnboardingManager.Instance.GoToNextStage();
      }
    }

    private void HandleTurnedTowardsFace(bool success) {
      _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnboardingReactToFace, HandleFaceReactionEndAnimationComplete);
    }

    private void HandleFaceReactionEndAnimationComplete(bool success) {
      OnboardingManager.Instance.GoToNextStage();
    }

    private void Update() {
      if (_CompletedInitialAnimation) {
        // Timed out we can't find a face
        if (_EndLookAtFacesTime_Sec < Time.time) {
          OnboardingManager.Instance.GoToNextStage();
        }
        // Found a face, so stop looking and react, exit this stage at end of reaction
        if (!_HasObservedFace) {
          if (_CurrentRobot.Faces.Count > 0) {
            // Reset timer so we stop on the reactions but can still timeout if not finding the face.
            _EndLookAtFacesTime_Sec = Time.time + _LookForFacesMaxTime_Sec;
            _CurrentRobot.ExecuteBehavior(BehaviorType.NoneBehavior);
            _CurrentRobot.TurnTowardsFacePose(_CurrentRobot.Faces[0], 4.3f, 10, HandleTurnedTowardsFace);
            _HasObservedFace = true;
          }
        }
      }
    }

  }

}
