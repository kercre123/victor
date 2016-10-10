using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ChallengeIdAttribute : PropertyAttribute {
  public ChallengeIdAttribute() {
  }
}

public class ChallengeDataList : ScriptableObject {

  private static ChallengeDataList _sInstance;

  public static void SetInstance(ChallengeDataList instance) {
    _sInstance = instance;
    SkillSystem.Instance.InitChallengeDefaults(instance);
  }

  public static ChallengeDataList Instance {
    get { return _sInstance; }
  }

  public ChallengeData[] ChallengeData;

  public ChallengeData GetChallengeDataById(string challengeId) {
    ChallengeData data = null;
    for (int i = 0; i < ChallengeData.Length; i++) {
      if (challengeId == ChallengeData[i].ChallengeID) {
        data = ChallengeData[i];
      }
    }
    return data;
  }

#if UNITY_EDITOR
  public IEnumerable<string> EditorGetIds() {
    List<string> idList = new List<string>();
    foreach (var data in ChallengeData) {
      idList.Add(data.ChallengeID);
    }
    return idList;
  }
#endif
}

