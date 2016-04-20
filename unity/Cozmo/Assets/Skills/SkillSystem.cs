using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

// Singleton for games to interface with to get current
// skill level values.
// This system calculates when thresholds should be calculated and changed.
public class SkillSystem {
  private static SkillSystem _sInstance;

  public event System.Action<int> OnLevelUp;


  private ChallengeData _CurrChallengeData;

  #region GameAPI

  public static SkillSystem Instance {
    get { 
      if (_sInstance == null) {
        _sInstance = new SkillSystem();
      }
      return _sInstance; 
    }
  }

  public void InitInstance() {
    SkillSystem.Instance.Init();
  }

  public void DestroyInstance() {
    SkillSystem.Instance.Destroy();
    _sInstance = null;
  }

  private void Init() {
    GameEventManager.Instance.OnGameEvent += HandleGameEvent;
  }

  private void Destroy() {
    GameEventManager.Instance.OnGameEvent -= HandleGameEvent;
  }

  public void StartGame(ChallengeData data) { 
    _CurrChallengeData = data;
  }

  public void EndGame() { 
    _CurrChallengeData = null;
  }

  public GameSkillLevelConfig GetSkillLevelConfig() {
    GameSkillData currSkillData = GetSkillDataForGame();
    if (currSkillData != null) {
      GameSkillConfig skillConfig = _CurrChallengeData.MinigameConfig.SkillConfig;
      GameSkillLevelConfig skillLevelConfig = skillConfig.GetCurrLevelConfig(currSkillData.LastLevel);
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

  public void HandleGameEvent(Anki.Cozmo.GameEvent cozEvent) {
    GameSkillData currSkillData = GetSkillDataForGame();
    if (currSkillData != null) {
      GameSkillConfig skillConfig = _CurrChallengeData.MinigameConfig.SkillConfig;
      if (skillConfig.GainChallengePointEvent == cozEvent) {
        currSkillData.WinPointsTotal++;
      }
      if (skillConfig.LoseChallengePointEvent == cozEvent) {
        currSkillData.LossPointsTotal++;
      }
      if (skillConfig.EvaluateLevelChangeEvent == cozEvent) {
        // See if things are pass the level up/down thresholds...
        // TODO: this needs to get the max of LastLevel player and Highest level cozmo
        GameSkillLevelConfig skillLevelConfig = GetSkillLevelConfig();
        if (skillLevelConfig != null) {
          // Reset the for next calculation if our percent has changed.
          float pointTotal = (float)(currSkillData.WinPointsTotal + currSkillData.LossPointsTotal);
          // Only evaluate after a certain number of points scored if desired so players can't just quit.
          bool thresholdPassed = true;
          if (skillConfig.UsePointThreshold && skillConfig.ComparedPointThreshold < pointTotal) {
            thresholdPassed = false;
          }
          if (thresholdPassed) {
            float winPercent = (currSkillData.WinPointsTotal / pointTotal);
            // We're losing too much, level up

            if (winPercent < skillLevelConfig.LowerBoundThreshold) {
              //  if new high, let the player know
              if (currSkillData.LastLevel + 1 < skillConfig.GetMaxLevel()) {
                bool newHighestLevel = currSkillData.LastLevel == currSkillData.HighestLevel;
                currSkillData.ChangeLevel(currSkillData.LastLevel + 1);
              
                DAS.Event("game.cozmoskill.levelup", _CurrChallengeData.ChallengeID, null, 
                  DASUtil.FormatExtraData(currSkillData.LastLevel.ToString() + "," + currSkillData.HighestLevel.ToString()));
                if (OnLevelUp != null && newHighestLevel) {
                  OnLevelUp(currSkillData.LastLevel);
                }
              }
            }
            // we're winning too much, level down
            else if (winPercent > skillLevelConfig.UpperBoundThreshold) {
              currSkillData.ChangeLevel(currSkillData.LastLevel - 1);
              DAS.Event("game.cozmoskill.leveldown", _CurrChallengeData.ChallengeID, null, 
                DASUtil.FormatExtraData(currSkillData.LastLevel.ToString() + "," + currSkillData.HighestLevel.ToString()));
            }
          }
        }
      }
    }
  }


}