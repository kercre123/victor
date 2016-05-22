using System;
using UnityEngine;

public class ChallengeDataList : ScriptableObject {
  private static ChallengeDataList _sInstance;

  public static void SetInstance(ChallengeDataList instance) {
    _sInstance = instance;
  }

  public static ChallengeDataList Instance {
    get { return _sInstance; }
  }

  public ChallengeData[] ChallengeData;
}

