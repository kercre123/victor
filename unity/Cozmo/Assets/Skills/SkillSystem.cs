using UnityEngine;
using System.Collections;

// Singleton for games to interface with to get current
// skill level values.
// This system calculates when thresholds should be calculated and changed.
public class SkillSystem {

  private static SkillSystem _sInstance;

  public static void SetInstance(SkillSystem instance) {
    _sInstance = instance;
  }

  public static SkillSystem Instance {
    get { return _sInstance; }
  }

  //private ChallengeData _ChallengeData;

  public void StartGame(ChallengeData data) { 
    //_ChallengeData = data;
    GameSkillConfig skill_config = data.MinigameConfig.SkillConfig;
    AnimationManager.Instance.AddAnimationEndedCallback(skill_config.EvaluateLevelChangeEvent, HandleEvaluateLevelChange);
    AnimationManager.Instance.AddAnimationEndedCallback(skill_config.GainChallengePointEvent, HandleGainChallengePoint);
    AnimationManager.Instance.AddAnimationEndedCallback(skill_config.LoseChallengePointEvent, HandleLoseChallengePoint);
  }

  public void EndGame(ChallengeData data) { 
    //_ChallengeData = null;
    GameSkillConfig skill_config = data.MinigameConfig.SkillConfig;
    AnimationManager.Instance.RemoveAnimationEndedCallback(skill_config.EvaluateLevelChangeEvent, HandleEvaluateLevelChange);
    AnimationManager.Instance.RemoveAnimationEndedCallback(skill_config.GainChallengePointEvent, HandleGainChallengePoint);
    AnimationManager.Instance.RemoveAnimationEndedCallback(skill_config.LoseChallengePointEvent, HandleLoseChallengePoint);
  }

  private void HandleEvaluateLevelChange(bool success) {
    DAS.Event("HandleEvaluateLevelChange", "HandleEvaluateLevelChange");
  }

  private void HandleGainChallengePoint(bool success) {
    DAS.Event("HandleGainChallengePoint", "HandleGainChallengePoint");
  }

  private void HandleLoseChallengePoint(bool success) {
    DAS.Event("HandleLoseChallengePoint", "HandleLoseChallengePoint");
  }

  /*StartGame(ChallengeData) // starts listening for when to update
  EndGame(ChallengeData) // stops listening
  GetSkillSet(ChallengeData) // gets whole map for curr level
  GetSkillVal(ChallengeData,skillname) // returns specific thing for level
  GetSkillLevel(ChallengeData)*/
}