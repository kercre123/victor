using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public class UnlockablesManager : MonoBehaviour {
  
  public static UnlockablesManager Instance { get; private set; }

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
    }
  }

  private Dictionary<Anki.Cozmo.UnlockIds, bool> _UnlockablesState = new Dictionary<Anki.Cozmo.UnlockIds, bool>();

  [SerializeField]
  private UnlockableInfoList _UnlockableInfoList;

  private void Start() {
    RobotEngineManager.Instance.OnRequestSetUnlockResult += HandleOnUnlockRequestSuccess;
  }

  // should be called when connected to the robot and loaded unlock info from the physical robot.
  public void OnConnectLoad(Dictionary<Anki.Cozmo.UnlockIds, bool> loadedUnlockables) {
    _UnlockablesState = loadedUnlockables;
    if (NothingUnlocked()) {
      SetDefaults();
    }
  }

  private bool NothingUnlocked() {
    foreach (KeyValuePair<Anki.Cozmo.UnlockIds, bool> kvp in _UnlockablesState) {
      if (kvp.Value) {
        return false;
      }
    }
    return true;
  }

  private void SetDefaults() {
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockableInfoList.UnlockableInfoData[i].DefaultUnlock) {
        _UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id] = true;
      }
    }
  }

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

  public List<UnlockableInfo> GetUnlocked(bool getExplicitOnly) {
    List<UnlockableInfo> unlocked = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id]) {
        if ((!getExplicitOnly) || (getExplicitOnly && _UnlockableInfoList.UnlockableInfoData[i].ExplicitUnlock)) {
          unlocked.Add(_UnlockableInfoList.UnlockableInfoData[i]);
        }
      }
    }
    return unlocked;
  }

  // explicit unlocks only.
  public List<UnlockableInfo> GetAvailableAndLockedExplicit() {
    List<UnlockableInfo> available = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id];
      bool isAvailable = IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id);
      if (locked && isAvailable && _UnlockableInfoList.UnlockableInfoData[i].ExplicitUnlock) {
        available.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return available;
  }

  // explicit unlocks only.
  public List<UnlockableInfo> GetUnavailableExplicit() {
    List<UnlockableInfo> unavailable = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id];
      bool isAvailable = IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id);
      if (locked && !isAvailable && _UnlockableInfoList.UnlockableInfoData[i].ExplicitUnlock) {
        unavailable.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unavailable;
  }

  public void TrySetUnlocked(Anki.Cozmo.UnlockIds id, bool unlocked) {
    RobotEngineManager.Instance.CurrentRobot.RequestSetUnlock(id, unlocked);
  }

  public bool IsUnlocked(Anki.Cozmo.UnlockIds id) {
    return _UnlockablesState[id];
  }

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

  private void HandleOnUnlockRequestSuccess(Anki.Cozmo.UnlockIds id, bool unlocked) {
    _UnlockablesState[id] = unlocked;
  }

}
