using Cozmo.Challenge;
using UnityEngine;
using System.Collections.Generic;
using DataPersistence;
using G2U = Anki.Cozmo.ExternalInterface;
using Anki.Cozmo.NVStorage;

/* 
Singleton for games to interface with to get current skill level values.
 This system calculates when thresholds should be calculated and changed.

It is a pure C# class not a monobehavior since it doesn't need to be on a scene.
However, that means it does need to be inited from elsewhere.

*/

public class SkillSystem {
  private static SkillSystem _sInstance;

  public event System.Action<int> OnLevelUp;

  private byte[] _CozmoHighestLevels;
  private ChallengeData _CurrChallengeData;
  private int _ChallengeIndex;

  public bool LevelingEnabled { get; set; }

  public enum SkillOverrideLevel {
    None,
    Min,
    Max,
  };
  private int _SkillOveride = -1;

  #region GameAPI

  public static SkillSystem Instance {
    get {
      if (_sInstance == null) {
        _sInstance = new SkillSystem();
      }
      return _sInstance;
    }
  }

  public void Initialize() {
    SkillSystem.Instance.Init();
  }

  public void InitChallengeDefaults(ChallengeDataList dataList) {
    // If the robot nvstorage hasn't already initialized a size, overwrite.
    if (_CozmoHighestLevels == null || _CozmoHighestLevels.Length < dataList.ChallengeData.Length) {
      SetCozmoHighestLevelsReached(null, 0);
    }
  }

  public void DestroyInstance() {
    SkillSystem.Instance.Destroy();
    _sInstance = null;
  }



  public void StartGame(ChallengeData data) {
    _CurrChallengeData = data;

    ChallengeData[] challengeList = ChallengeDataList.Instance.ChallengeData;
    for (int i = 0; i < challengeList.Length; ++i) {
      if (challengeList[i].ChallengeID == _CurrChallengeData.ChallengeID) {
        _ChallengeIndex = i;
        break;
      }
    }
  }

  public void EndGame() {
    _CurrChallengeData = null;
    _ChallengeIndex = -1;
  }

  // If player last level was 10 but on a new cozmo thats only level 3. That cozmo should play at level 3
  public int GetCozmoSkillLevel(GameSkillData playerSkill) {
    if (_SkillOveride >= 0) {
      return _SkillOveride;
    }
    if (_CozmoHighestLevels.Length > _ChallengeIndex) {
      return Mathf.Min(_CozmoHighestLevels[_ChallengeIndex], playerSkill.LastLevel);
    }
    else {
      DAS.Warn("SkillSystem.GetCozmoSkillLevel", "GetCozmoSkillLevel out of range Exception, Something has gone terribly wrong.");
      return 0;
    }
  }

  public void SetSkillOverride(SkillOverrideLevel level) {
    if (level == SkillOverrideLevel.None) {
      _SkillOveride = -1;
    }
    else if (level == SkillOverrideLevel.Min) {
      _SkillOveride = 0;
    }
    else if (level == SkillOverrideLevel.Max && _CurrChallengeData != null) {
      GameSkillConfig skillConfig = _CurrChallengeData.ChallengeConfig.SkillConfig;
      _SkillOveride = skillConfig.GetMaxLevel() - 1;
    }
  }

  public GameSkillLevelConfig GetSkillLevelConfig() {
    GameSkillData currSkillData = GetSkillDataForGame();
    if (currSkillData != null) {
      GameSkillConfig skillConfig = _CurrChallengeData.ChallengeConfig.SkillConfig;
      GameSkillLevelConfig skillLevelConfig = skillConfig.GetCurrLevelConfig(GetCozmoSkillLevel(currSkillData));

      return skillLevelConfig;
    }
    return null;
  }

  public float GetSkillVal(string skillName) {
    GameSkillLevelConfig skillLevelConfig = GetSkillLevelConfig();
    float val = 0.0f;
    if (skillLevelConfig != null) {
      skillLevelConfig.SkillMap.TryGetValue(skillName, out val);
    }
    return val;
  }

  public bool HasSkillVal(string skillName) {
    GameSkillLevelConfig skillLevelConfig = GetSkillLevelConfig();
    if (skillLevelConfig != null) {
      return skillLevelConfig.SkillMap.ContainsKey(skillName);
    }
    return false;
  }

  #endregion

  #region DebugMenuAPI

  public bool GetDebugSkillsForGame(out int profileCurrSkill, out int profileHighSkill, out int robotHighSkill) {
    profileCurrSkill = -1;
    profileHighSkill = -1;
    robotHighSkill = -1;
    GameSkillData profileData = GetSkillDataForGame();
    if (profileData != null && _ChallengeIndex >= 0) {
      profileCurrSkill = profileData.LastLevel;
      profileHighSkill = profileData.HighestLevel;
      robotHighSkill = _CozmoHighestLevels[_ChallengeIndex];
    }
    return profileData != null;
  }

  public bool SetDebugSkillsForGame(int profileCurrSkill, int profileHighSkill, int robotHighSkill) {
    GameSkillData profileData = GetSkillDataForGame();
    if (profileData != null && _ChallengeIndex >= 0) {
      profileData.LastLevel = profileCurrSkill;
      profileData.HighestLevel = profileHighSkill;
      _CozmoHighestLevels[_ChallengeIndex] = (byte)robotHighSkill;

      UpdateHighestSkillsOnRobot();
      DataPersistenceManager.Instance.Save();
    }
    return profileData != null;
  }

  public void DebugEraseStorage() {
    RobotEngineManager.Instance.Message.NVStorageEraseEntry = new G2U.NVStorageEraseEntry();
    RobotEngineManager.Instance.Message.NVStorageEraseEntry.tag = NVEntryTag.NVEntry_GameSkillLevels;
    RobotEngineManager.Instance.SendMessage();
  }

  #endregion

  private GameSkillData GetSkillDataForGame() {
    // In the event we've never played this game before and they have old save data.
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CozmoSkillLevels == null) {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CozmoSkillLevels = new Dictionary<string, GameSkillData>();
    }
    if (_CurrChallengeData) {
      if (!DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CozmoSkillLevels.ContainsKey(_CurrChallengeData.ChallengeID)) {
        DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CozmoSkillLevels.Add(_CurrChallengeData.ChallengeID, new GameSkillData());
      }
      return DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CozmoSkillLevels[_CurrChallengeData.ChallengeID];
    }
    return null;
  }

  public void HandleGameEvent(GameEventWrapper cozEvent) {
    // During profileless games with multiple players
    // or Memory Match solo mode don't count that
    // as being better at cozmo so don't level up.
    if (!LevelingEnabled) {
      return;
    }
    GameSkillData currSkillData = GetSkillDataForGame();
    if (currSkillData != null) {
      bool playerWin = false;
      if (cozEvent is ChallengeGameEvent) {
        ChallengeGameEvent challengeEvent = (ChallengeGameEvent)cozEvent;
        playerWin = challengeEvent.PlayerWin;
      }
      GameSkillConfig skillConfig = _CurrChallengeData.ChallengeConfig.SkillConfig;
      if (skillConfig.IsChallengePointEvent(cozEvent.GameEventEnum)) {
        // Since this controls Cozmo's skill level, these are named from Cozmo's PoV unlike the event names.
        if (playerWin) {
          currSkillData.LossPointsTotal++;
        }
        else {
          currSkillData.WinPointsTotal++;
        }
      }
      // In the event we quit out early and didn't reach an evaluate event, force a clear
      if (skillConfig.IsResetPointEvent(cozEvent.GameEventEnum)) {
        currSkillData.ResetPoints();
      }

      if (skillConfig.IsLevelChangeEvent(cozEvent.GameEventEnum)) {
        // See if things are pass the level up/down thresholds...
        GameSkillLevelConfig skillLevelConfig = GetSkillLevelConfig();
        if (skillLevelConfig != null) {
          // Reset the for next calculation if our percent has changed.
          float pointTotal = (float)(currSkillData.WinPointsTotal + currSkillData.LossPointsTotal);
          // Only evaluate after a certain number of points scored if desired so players can't just quit.
          bool thresholdPassed = true;
          if (skillConfig.UsePointThreshold && pointTotal < skillConfig.ComparedPointThreshold) {
            thresholdPassed = false;
          }
          if (thresholdPassed) {
            float winPercent = (currSkillData.WinPointsTotal / pointTotal);

            if (winPercent < skillLevelConfig.LowerBoundThreshold) {
              int cozmoSkillLevel = GetCozmoSkillLevel(currSkillData);

              //  if new high, let the player know
              if (cozmoSkillLevel + 1 < skillConfig.GetMaxLevel()) {
                // We're losing too much, level up
                RewardedActionManager.Instance.NewSkillChange += 1;

                bool newHighestLevel = cozmoSkillLevel == currSkillData.HighestLevel;
                // this is explicity player profile based, ignore robot level.
                if (cozmoSkillLevel == currSkillData.LastLevel) {
                  currSkillData.ChangeLevel(currSkillData.LastLevel + 1);
                }

                // Extra checking since we're seeing an error here
                if (_ChallengeIndex < 0 || _ChallengeIndex >= _CozmoHighestLevels.Length) {
                  DAS.Error("SkillSystem.HandleGameEvent", "GameEvent " + cozEvent.GameEventEnum +
                    " can't be handled with _ChallengeIndex " + _ChallengeIndex +
                    " and _CozmoHighestLevels.Length " + _CozmoHighestLevels.Length);
                }

                if (_CozmoHighestLevels[_ChallengeIndex] < currSkillData.LastLevel) {
                  _CozmoHighestLevels[_ChallengeIndex]++;
                }
                UpdateHighestSkillsOnRobot();

                DAS.Event("game.cozmoskill.levelup", _CurrChallengeData.ChallengeID,
                  DASUtil.FormatExtraData(cozmoSkillLevel.ToString() + "," + currSkillData.HighestLevel.ToString()));

                if (OnLevelUp != null && newHighestLevel) {
                  OnLevelUp(currSkillData.LastLevel);
                  DataPersistenceManager.Instance.Save();
                }
              }
            }
            // we're winning too much, level down
            else if (winPercent > skillLevelConfig.UpperBoundThreshold) {
              int nextLevel = currSkillData.LastLevel - 1;
              if (nextLevel >= 0) {
                RewardedActionManager.Instance.NewSkillChange -= 1;
                // cozmosHighestRobotLevel never levels down
                DAS.Event("game.cozmoskill.leveldown", _CurrChallengeData.ChallengeID,
                  DASUtil.FormatExtraData(currSkillData.LastLevel.ToString() + "," + currSkillData.HighestLevel.ToString()));
              }
              // Change level will cap and reset points
              currSkillData.ChangeLevel(nextLevel);
            }
            else {
              currSkillData.ResetPoints();
            }
          }
        }
      }
    }
  }

  private void Destroy() {
    GameEventManager.Instance.OnGameEvent -= HandleGameEvent;

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.NVStorageOpResult>(HandleNVStorageOpResult);
  }

  private void Init() {
    GameEventManager.Instance.OnGameEvent += HandleGameEvent;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.NVStorageOpResult>(HandleNVStorageOpResult);

    _ChallengeIndex = -1;
    _CurrChallengeData = null;
    SetCozmoHighestLevelsReached(null, 0);
    LevelingEnabled = true;
  }

  private void HandleRobotConnected(Anki.Cozmo.ExternalInterface.RobotConnectionResponse message) {
    if (message.result == Anki.Cozmo.RobotConnectionResult.Success) {
      RobotEngineManager.Instance.Message.NVStorageReadEntry = new G2U.NVStorageReadEntry();
      RobotEngineManager.Instance.Message.NVStorageReadEntry.tag = NVEntryTag.NVEntry_GameSkillLevels;
      RobotEngineManager.Instance.SendMessage();
    }
  }
  // if this was an failure we're never going to get the result we
  private void HandleNVStorageOpResult(G2U.NVStorageOpResult opResult) {
    if (opResult.op == NVOperation.NVOP_READ &&
        opResult.tag == NVEntryTag.NVEntry_GameSkillLevels) {
      if (opResult.result == NVResult.NV_OKAY) {
        SetCozmoHighestLevelsReached(opResult.data, opResult.data.Length);
      }
      else {
        // write out defaults so we have some 0s for next time,
        // This was likely the first time and was just a "not found"
        UpdateHighestSkillsOnRobot();
      }
    }
  }

  private void SetCozmoHighestLevelsReached(byte[] robotData, int robotDataLen) {
    // RobotData is just highest level in challengeList order
    ChallengeDataList challengeList = ChallengeDataList.Instance;
    // default if starting on the wrong scene or in factory 
    int numChallenges = 1;
    if (challengeList != null) {
      numChallenges = challengeList.ChallengeData.Length;
    }
    numChallenges = Mathf.Max(robotDataLen, numChallenges);
    // Usually hits the default from the Challenge List loading
    if (_CozmoHighestLevels == null) {
      _CozmoHighestLevels = new byte[numChallenges];
    }
    else {
      // if we connected to the robot first, but a patch installed new challenges.
      // or the defaults were in initalized already, just keep them
      byte[] oldValues = _CozmoHighestLevels;
      _CozmoHighestLevels = new byte[numChallenges];
      System.Array.Copy(oldValues, _CozmoHighestLevels, oldValues.Length);
    }
    // first time init
    if (robotData != null && robotDataLen > 0) {
      DAS.Info("SkillSystem.SetCozmoHighestLevelsReached", "Copying " + robotDataLen + " elements.");
      System.Array.Copy(robotData, _CozmoHighestLevels, robotDataLen);
    }
  }

  private void UpdateHighestSkillsOnRobot() {
    // Write to updated array...
    RobotEngineManager.Instance.Message.NVStorageWriteEntry = new G2U.NVStorageWriteEntry();
    RobotEngineManager.Instance.Message.NVStorageWriteEntry.tag = Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_GameSkillLevels;
    RobotEngineManager.Instance.Message.NVStorageWriteEntry.data = new byte[_CozmoHighestLevels.Length];
    System.Array.Copy(_CozmoHighestLevels, RobotEngineManager.Instance.Message.NVStorageWriteEntry.data, _CozmoHighestLevels.Length);
    RobotEngineManager.Instance.SendMessage();
  }


}