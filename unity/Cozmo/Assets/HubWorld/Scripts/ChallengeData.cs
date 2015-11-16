using UnityEngine;
using System.Collections;

public class ChallengeData {
  // the mini game prefab to load for this challenge
  public string MinigamePrefabPath;

  // the key used to find this specific challenge
  public string ChallengeID;

  // the key used to get the actual localized string for the
  // challenge title shown to the player.
  public string ChallengeTitleKey;

  // The set of requirements needed to unlock this challenge
  public ChallengeRequirements ChallengeReqs;

  // JSON passed to the specific mini game to be serialized
  // for this specific challenge.
  public string MinigameParametersJSON;
}
