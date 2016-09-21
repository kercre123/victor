using UnityEngine;
using System.Collections;
using Anki.Cozmo;

public class RequestGameConfig : ScriptableObject {
  [ChallengeId]
  public string ChallengeID;
  public BehaviorGameFlag RequestBehaviorGameFlag;
  public SerializableAnimationTrigger RequestAcceptedAnimationTrigger;
}
