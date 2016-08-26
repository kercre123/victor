using UnityEngine;
using Anki.Cozmo;
using System;
using System.Collections;
using System.Collections.Generic;

using Anki.Cozmo.ExternalInterface;


public class UnlockablesManager : MonoBehaviour {

  public static UnlockablesManager Instance { get; private set; }

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
      InitializeUnlockablesState();
    }
  }

  public Action<UnlockId> OnSparkStarted;
  public Action<CoreUpgradeDetailsDialog> OnSparkComplete;
  public Action<Anki.Cozmo.UnlockId, bool> OnUnlockPopupRequested;

  public bool UnlocksLoaded { get { return _UnlocksLoaded; } }

  private bool _UnlocksLoaded = false;

  private Dictionary<Anki.Cozmo.UnlockId, bool> _UnlockablesState = new Dictionary<Anki.Cozmo.UnlockId, bool>();

  [SerializeField]
  private UnlockableInfoList _UnlockableInfoList;

  private List<Anki.Cozmo.UnlockId> _NewUnlocks = new List<UnlockId>();

  // index is the face slot, value is the unlockID;
  [SerializeField]
  private int[] _FaceSlotUnlockMap;

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

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleOnUnlockRequestSuccess);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.UnlockStatus>(HandleUnlockStatus);
  }

  // should be called when connected to the robot and loaded unlock info from the physical robot.
  public void OnConnectLoad(Dictionary<Anki.Cozmo.UnlockId, bool> loadedUnlockables) {
    DAS.Info("UnlockablesManager.OnConnectLoad", "Unlocks loaded from robot / engine");
    _UnlockablesState = loadedUnlockables;
    _UnlocksLoaded = true;
  }

  private bool NothingUnlocked() {
    foreach (KeyValuePair<Anki.Cozmo.UnlockId, bool> kvp in _UnlockablesState) {
      if (kvp.Value) {
        return false;
      }
    }
    return true;
  }

  public bool IsFaceSlotUnlocked(int faceSlot) {
    if (faceSlot >= _FaceSlotUnlockMap.Length) {
      DAS.Error("UnlockablesManager.IsFaceSlotUnlocked", "Face slot out of range " + faceSlot);
    }
    return IsUnlocked((Anki.Cozmo.UnlockId)_FaceSlotUnlockMap[faceSlot]);
  }

  public KeyValuePair<string, int> FaceUnlockCost(int faceSlot) {
    UnlockableInfo unlockInfo = System.Array.Find(_UnlockableInfoList.UnlockableInfoData, (obj) => (int)obj.Id.Value == _FaceSlotUnlockMap[faceSlot]);
    KeyValuePair<string, int> costInfo = new KeyValuePair<string, int>("", 0);
    if (unlockInfo != null) {
      costInfo = new KeyValuePair<string, int>(unlockInfo.UpgradeCostItemId, unlockInfo.UpgradeCostAmountNeeded);
    }
    else {
      DAS.Error("UnlockablesManager.FaceUnlockCost", "Could not find faceslot " + faceSlot);
    }
    return costInfo;
  }

  public int FaceSlotsSize() {
    return _FaceSlotUnlockMap.Length;
  }

  public void UnlockNextAvailableFaceSlot() {
    for (int i = 0; i < _FaceSlotUnlockMap.Length; ++i) {
      if (!IsUnlocked((Anki.Cozmo.UnlockId)_FaceSlotUnlockMap[i])) {
        TrySetUnlocked((UnlockId)_FaceSlotUnlockMap[i], true);
        return;
      }
    }
    DAS.Warn("UnlockablesManager.UnlockNextAvailableFaceSlot", "No slots left!");
  }


  // Should only be called before connecting to robot, robot will overwrite these
  public void InitializeUnlockablesState() {
    DAS.Info("UnlockablesManager.InitializeUnlockablesState", "InitializeUnlockablesState");
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockablesState.ContainsKey(_UnlockableInfoList.UnlockableInfoData[i].Id.Value) == false) {
        _UnlockablesState.Add(_UnlockableInfoList.UnlockableInfoData[i].Id.Value, false);
      }
    }
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleOnUnlockRequestSuccess);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.UnlockStatus>(HandleUnlockStatus);
    RobotEngineManager.Instance.SendRequestUnlockDataFromBackup();
  }

  public List<UnlockableInfo> GetUnlockablesByType(UnlockableType unlockableType) {
    List<UnlockableInfo> unlockables = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      if (_UnlockableInfoList.UnlockableInfoData[i].UnlockableType == unlockableType) {
        unlockables.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unlockables;
  }

  public List<UnlockableInfo> GetUnlocked() {
    List<UnlockableInfo> unlocked = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool neverAvailable = _UnlockableInfoList.UnlockableInfoData[i].NeverAvailable;
      if (_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value]) {
        if (!neverAvailable) {
          unlocked.Add(_UnlockableInfoList.UnlockableInfoData[i]);
        }
      }
    }
    return unlocked;
  }

  // explicit unlocks only.
  public List<UnlockableInfo> GetAvailableAndLocked() {
    List<UnlockableInfo> available = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value];
      bool neverAvailable = _UnlockableInfoList.UnlockableInfoData[i].NeverAvailable;
      bool isAvailable = !neverAvailable && IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id.Value);
      if (locked && isAvailable) {
        available.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return available;
  }

  // explicit unlocks only.
  public List<UnlockableInfo> GetUnavailable() {
    List<UnlockableInfo> unavailable = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool neverAvailable = _UnlockableInfoList.UnlockableInfoData[i].NeverAvailable;
      bool locked = !_UnlockablesState[_UnlockableInfoList.UnlockableInfoData[i].Id.Value];
      bool isAvailable = IsUnlockableAvailable(_UnlockableInfoList.UnlockableInfoData[i].Id.Value);
      if (!neverAvailable && locked && !isAvailable) {
        unavailable.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return unavailable;
  }

  public List<UnlockableInfo> GetNeverAvailable() {
    List<UnlockableInfo> comingSoon = new List<UnlockableInfo>();
    for (int i = 0; i < _UnlockableInfoList.UnlockableInfoData.Length; ++i) {
      bool neverAvailable = _UnlockableInfoList.UnlockableInfoData[i].NeverAvailable;
      if (neverAvailable) {
        comingSoon.Add(_UnlockableInfoList.UnlockableInfoData[i]);
      }
    }
    return comingSoon;
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
    bool unlocked = false;
    if (_UnlockablesState.TryGetValue(id, out unlocked)) {
      return unlocked;
    }
    else {
      DAS.Error("UnlockablesManager.IsUnlocked", string.Format("_UnlockablesState does not contain {0}", id));
      return false;
    }
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

    if (unlockableInfo.NeverAvailable) {
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
    // Only update from backup if we aren't connected to a real cozmo.
    if ((message.fromBackup) && (RobotEngineManager.Instance.CurrentRobot != null)) {
      DAS.Info("UnlockablesManager.HandleUnlockStatus", "Backup Data Ignored, Already have Real Robot Data");
      return;
    }
    if (message.fromBackup) {
      DAS.Info("UnlockablesManager.HandleUnlockStatus", "Backup Data Accepted, No Robot Connected");
    }

    Dictionary<Anki.Cozmo.UnlockId, bool> loadedUnlockables = new Dictionary<UnlockId, bool>();
    for (int i = 0; i < (int)Anki.Cozmo.UnlockId.Count; ++i) {
      loadedUnlockables.Add((UnlockId)i, false);
    }

    for (int i = 0; i < message.unlocks.Length; ++i) {
      DAS.Info("UnlockablesManager.HandleUnlockStatus ", message.unlocks[i].ToString());
      loadedUnlockables[message.unlocks[i]] = true;
    }
    OnConnectLoad(loadedUnlockables);
  }

  private void HandleOnUnlockRequestSuccess(Anki.Cozmo.ExternalInterface.RequestSetUnlockResult resultMessage) {
    _UnlockablesState[resultMessage.unlockID] = resultMessage.unlocked;
    if (resultMessage.unlocked) {
      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnUnlockableEarned, resultMessage.unlockID));
      _NewUnlocks.Add(resultMessage.unlockID);

      // Trigger soft spark in the engine if the unlock was an action
      if (GetUnlockableInfo(resultMessage.unlockID).UnlockableType == UnlockableType.Action) {
        RobotEngineManager.Instance.Message.BehaviorManagerMessage =
        Singleton<BehaviorManagerMessage>.Instance.Initialize(
          Convert.ToByte (RobotEngineManager.Instance.CurrentRobotID),
          Singleton<SparkUnlocked>.Instance.Initialize (resultMessage.unlockID)
        );
        RobotEngineManager.Instance.SendMessage();
      }
    }
  }

}
