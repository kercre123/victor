using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    public Dictionary<int, string> _FaceNameDictionary = new Dictionary<int, string>();

    private string[] _ReactionBank = {
      AnimationName.kFaceEnrollmentReaction_00,
      AnimationName.kFaceEnrollmentReaction_01,
      AnimationName.kFaceEnrollmentReaction_02,
      AnimationName.kFaceEnrollmentReaction_03,
      AnimationName.kFaceEnrollmentReaction_04
    };

    private string _NameForSpace;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
    }

    protected override void InitializeView(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.InitializeView(newView, data);

      // create enter name dialog
    }

    private void HandleNameEntered(string name) {
      _NameForSpace = name;
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.LookingStraight);

      // create enrollment view
    }

    private void HandleObservedNewFace(int id, Vector3 pos, Quaternion rot) {
      // we've successfully enrolled a new face.
      CurrentRobot.AssignNameToFace(id, _NameForSpace);
      PlayFaceReactionAnimation(id);
    }

    private void PlayFaceReactionAnimation(int faceId) {
      DAS.Debug(this, "Attempt to Play Face Reaction Animation - FaceId: " + faceId);
      CurrentRobot.PrepareFaceNameAnimation(faceId, _FaceNameDictionary[faceId]);
      CurrentRobot.SendAnimation(_ReactionBank[Random.Range(0, _ReactionBank.Length)], HandleReactionDone);
    }

    private void HandleReactionDone(bool success) {
      base.RaiseMiniGameWin();
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

  }

}
