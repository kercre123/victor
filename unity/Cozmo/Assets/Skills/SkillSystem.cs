using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

// Singleton for games to interface with to get current
// skill level values.
// This system calculates when thresholds should be calculated and changed.
public class SkillSystem {

  private static SkillSystem _sInstance;

  private SkillSystem() {
    GameEventManager.Instance.OnGameEvent += HandleGameEvent;
  }

  public static SkillSystem Instance {
    get { 
      if (_sInstance == null) {
        _sInstance = new SkillSystem();
      }
      return _sInstance; 
    }
  }

  private ChallengeData _CurrChallengeData;

  public void StartGame(ChallengeData data) { 
    _CurrChallengeData = data;
  }

  public void EndGame(ChallengeData data) { 
    //_CurrChallengeData = null;
  }

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
    GameSkillData curr_skill_data = GetSkillDataForGame();
    if (curr_skill_data != null) {
      GameSkillConfig skill_config = _CurrChallengeData.MinigameConfig.SkillConfig;
      if (skill_config.GainChallengePointEvent == cozEvent) {
        curr_skill_data.WinPointsTotal++;
      }
      if (skill_config.LoseChallengePointEvent == cozEvent) {
        curr_skill_data.LossPointsTotal++;
      }
      if (skill_config.EvaluateLevelChangeEvent == cozEvent) {
        // See if things are pass the level up/down thresholds...
        // TODO: this needs to get the max of LastLevel player and Highest level cozmo
        GameSkillLevelConfig skill_level_config = GetSkillLevelConfig();
        if (skill_level_config != null) {
          // Reset the for next calculation if our percent has changed.
          float point_total = (float)(curr_skill_data.WinPointsTotal + curr_skill_data.LossPointsTotal);
          float win_percent = (curr_skill_data.WinPointsTotal / point_total);
          DAS.Warn("SkillSystem.EvaluateLevelChangeEvent Point Total", "SkillSystem.EvaluateLevelChangeEvent2 " + point_total);
          DAS.Warn("SkillSystem.EvaluateLevelChangeEvent Point Percent", "SkillSystem.EvaluateLevelChangeEvent3 " + win_percent);
          DAS.Warn("SkillSystem.EvaluateLevelChangeEvent Level Up threshold", "SkillSystem.EvaluateLevelChangeEvent3 " + skill_level_config.LevelUpMinThreshold);
          if (win_percent > skill_level_config.LevelUpMinThreshold) {
            // TODO: if new high, put in an optional popup here.
            curr_skill_data.ChangeLevel(curr_skill_data.LastLevel + 1);
          }
          if (win_percent < skill_level_config.LevelDownMaxThreshold) {
            curr_skill_data.ChangeLevel(curr_skill_data.LastLevel - 1);
          }
        }
      }
      // Just look at the file for testing
// TODO: DELETE THIS!!
      DataPersistence.DataPersistenceManager.Instance.Save();
    }
  }

  public GameSkillLevelConfig GetSkillLevelConfig() {
    GameSkillData curr_skill_data = GetSkillDataForGame();
    if (curr_skill_data != null) {
      GameSkillConfig skill_config = _CurrChallengeData.MinigameConfig.SkillConfig;
      GameSkillLevelConfig skill_level_config = skill_config.GetCurrLevelConfig(curr_skill_data.LastLevel);
      return skill_level_config;
    }
    return null;
  }

  public float GetSkillVal(string skill_name) {
    GameSkillLevelConfig skill_level_config = GetSkillLevelConfig();
    float val = 0.0f;
    if (skill_level_config != null) {
      skill_level_config.SkillMap.TryGetValue(skill_name, out val);
    }
    return val;
  }

  public bool HasSkillVal(string skill_name) {
    GameSkillLevelConfig skill_level_config = GetSkillLevelConfig();
    if (skill_level_config != null) {
      return skill_level_config.SkillMap.ContainsKey(skill_name);
    }
    return false;
  }
}