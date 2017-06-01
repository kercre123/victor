using Anki.Cozmo;
using Cozmo.Challenge;
using UnityEngine;

public class RequestGameConfig : ScriptableObject {
  [ChallengeId]
  public string ChallengeID;
  public UnlockableInfo.SerializableUnlockIds RequestGameID;
  public SerializableAnimationTrigger RequestAcceptedAnimationTrigger;
}
