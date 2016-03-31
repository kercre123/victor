using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public class UnlockableInfoList : ScriptableObject {
  public UnlockableInfo[] UnlockableInfoData;
}

public class UnlockablesManager : MonoBehaviour {
  private Dictionary<Anki.Cozmo.UnlockIds, bool> _UnlockablesState = new Dictionary<Anki.Cozmo.UnlockIds, bool>();

  [SerializeField]
  private UnlockableInfoList _UnlockableInfoList;

  public List<UnlockableInfo> GetUnlockedGames() {
    List<UnlockableInfo> unlocked = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id] &&
          _UnlockableInfoList.UnlockableInfoData[i].UnlockableType == UnlockableType.Game) {
        unlocked.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unlocked;
  }

  public List<UnlockableInfo> GetUnlocked() {
    List<UnlockableInfo> unlocked = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id]) {
        unlocked.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unlocked;
  }

  public List<UnlockableInfo> GetAvailableAndLocked() {
    List<UnlockableInfo> available = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id];
      bool isAvailable = IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id);
      if (locked && isAvailable) {
        available.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return available;
  }

  public List<UnlockableInfo> GetUnavailable() {
    List<UnlockableInfo> unavailable = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id];
      bool isAvailable = IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id);
      if (locked && !isAvailable) {
        unavailable.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unavailable;
  }

  // is something unlocked. works for implicit unlocks.
  public bool IsUnlocked(Anki.Cozmo.UnlockIds id) {
    return _UnlockablesState[id];
  }

  // is something available to be unlocked. only works for explicit unlocks.
  public bool IsUnlockableAvailable(Anki.Cozmo.UnlockIds id) {
    if (_UnlockablesState[id]) {
      return true;
    }

    UnlockableInfo unlockableInfo = Array.Find(_UnlockableInfoList.UnlockableInfoData, x => x.Id == id);

    if (unlockableInfo == null) {
      DAS.Error(this, "Invalid unlockable id " + id);
      return false;
    }

    for (int i = 0; i < unlockableInfo.Prerequisites.Length; ++i) {
      if (!_UnlockablesState[unlockableInfo.Prerequisites[i]]) {
        if (unlockableInfo.AnyPrereqUnlock == false) {
          return false;
        }
      }
      else {
        if (unlockableInfo.AnyPrereqUnlock) {
          return true;
        }
      }
    }

    return true;

  }

}
