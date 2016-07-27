using UnityEngine;
using Anki.Cozmo;
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

  public Action OnNewUnlock;

  private Dictionary<Anki.Cozmo.UnlockId, bool> _UnlockablesState = new Dictionary<Anki.Cozmo.UnlockId, bool>();

  [SerializeField]
  private UnlockableInfoList _UnlockableInfoList;

  [SerializeField]
  private List<Anki.Cozmo.UnlockId> _NewUnlocks = new List<UnlockId>();

  public bool IsNewUnlock(Anki.Cozmo.UnlockId uID) {
    return _NewUnlocks.Contains(uID);
  }

  /// <summary>
  /// Resolves the new unlocks. Currently just clears the list, but in the future will also save relevant
  /// information regarding actually seeing an unlock or not.
  /// </summary>
  public void ResolveNewUnlocks() {
    _NewUnlocks.Clear();
  }

  private void Start() {
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleOnUnlockRequestSuccess);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.UnlockStatus>(HandleUnlockStatus);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleOnUnlockRequestSuccess);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.UnlockStatus>(HandleUnlockStatus);
  }

  // should be called when connected to the robot and loaded unlock info from the physical robot.
  public void OnConnectLoad(Dictionary<Anki.Cozmo.UnlockId, bool> loadedUnlockables) {
    _UnlockablesState = loadedUnlockables;
  }

  private bool NothingUnlocked() {
    foreach (KeyValuePair<Anki.Cozmo.UnlockId, bool> kvp in _UnlockablesState) {
      if (kvp.Value) {
        return false;
      }
    }
    return true;
  }

  public List<UnlockableInfo> GetUnlockedGames() {
    List<UnlockableInfo> unlocked = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value] &&
          _UnlockableInfoList.UnlockableInfoData[i].UnlockableType == UnlockableType.Game) {
        unlocked.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unlocked;
  }

  public List<UnlockableInfo> GetUnlocked(bool getExplicitOnly) {
    List<UnlockableInfo> unlocked = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value]) {
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
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value];
      bool neverAvailable = _UnlockableInfoList.UnlockableInfoData[i].NeverAvailable;
      bool isAvailable = !neverAvailable && IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id.Value);
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
      bool neverAvailable = _UnlockableInfoList.UnlockableInfoData[i].NeverAvailable;
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value];
      bool isAvailable = IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id.Value);
      if (neverAvailable
          || (locked && !isAvailable && _UnlockableInfoList.UnlockableInfoData[i].ExplicitUnlock)) {
        unavailable.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unavailable;
  }

  public UnlockableInfo GetUnlockableInfo(Anki.Cozmo.UnlockId id) {
    UnlockableInfo info = Array.Find(_UnlockableInfoList.UnlockableInfoData, x => x.Id.Value == id);
    if (info == null) {
      DAS.Error(this, "Invalid unlockable id " + id);
      return null;
    }
    return info;
  }

  public void TrySetUnlocked(Anki.Cozmo.UnlockId id, bool unlocked) {
    RobotEngineManager.Instance.CurrentRobot.RequestSetUnlock(id, unlocked);
  }

  public bool IsUnlocked(Anki.Cozmo.UnlockId id) {
    return _UnlockablesState[id];
  }

  public bool IsUnlockableAvailable(Anki.Cozmo.UnlockId id) {
    if (_UnlockablesState[id]) {
      return true;
    }

    UnlockableInfo unlockableInfo = Array.Find(_UnlockableInfoList.UnlockableInfoData, x => x.Id.Value == id);

    if (unlockableInfo == null) {
      DAS.Error(this, "Invalid unlockable id " + id);
      return false;
    }

    for (int i = 0; i < unlockableInfo.Prerequisites.Length; ++i) {
      if (!_UnlockablesState[unlockableInfo.Prerequisites[i].Value]) {
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

  private void HandleUnlockStatus(Anki.Cozmo.ExternalInterface.UnlockStatus message) {
    Dictionary<Anki.Cozmo.UnlockId, bool> loadedUnlockables = new Dictionary<UnlockId, bool>();
    for (int i = 0; i < message.unlocks.Length; ++i) {
      loadedUnlockables.Add(message.unlocks[i].unlockID, message.unlocks[i].unlocked);
    }
    OnConnectLoad(loadedUnlockables);
  }

  private void HandleOnUnlockRequestSuccess(Anki.Cozmo.ExternalInterface.RequestSetUnlockResult resultMessage) {
    _UnlockablesState[resultMessage.unlockID] = resultMessage.unlocked;
    if (resultMessage.unlocked) {
      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnUnlockableEarned, resultMessage.unlockID));
      _NewUnlocks.Add(resultMessage.unlockID);
    }
  }

}
