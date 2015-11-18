using UnityEngine;
using System.Collections;
using AskCozmo;
using System;

public class ChallengeData : ScriptableObject {
  // the mini game prefab to load for this challenge
  public GameObject MinigamePrefab;

  // the key used to find this specific challenge
  public string ChallengeID;

  // the key used to get the actual localized string for the
  // challenge title shown to the player.
  public string ChallengeTitleKey;

  // The set of requirements needed to unlock this challenge
  public ChallengeRequirements ChallengeReqs;

  // string path to MinigameConfig
  public string MinigameConfigPath;
}
