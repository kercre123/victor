using UnityEngine;
using System.Collections;
using Anki.Cozmo;

public class RequestGameConfig : ScriptableObject {
  [ChallengeId]
  public string ChallengeID;
  public BehaviorGameFlag RequestBehaviorGameFlag;
  public SerializableAnimationTrigger RequestAcceptedAnimationTrigger;

  [Tooltip("Weight this config will be selected with no penelties. Should be > 0")]
  public float BaseWeight = 1.0f;

  [Tooltip("Max time penalty after a request is rejected")]
  public float RejectRepeatPenaltyMaxTime_Sec = 60 * 6;

  [Tooltip("Increase Rejection Penalty Each Time")]
  public float ProgressiveMultiplier = 2.0f;

  [Tooltip("Bonus Score for being included in Daily Bonus")]
  public float DailyGoalIncludedBonusWeight = 1.5f;

  public bool WasSelectedPrevious { get; set; }
  private int _TimesRejected = 0;
  private float _StartTimeWait = -1;

  public void RejectGame() {
    _StartTimeWait = Time.time;
    _TimesRejected++;
  }

  public float GetCurrentScore(bool isChallengeDailyGoal, bool isDebugRequest) {
    // NEVER return the same game twice in a row.
    if (WasSelectedPrevious) {
      return 0.0f;
    }
    // Always reject games that are on cooldown
    if (_StartTimeWait > 0 && !isDebugRequest) {
      // Stacking penalty timer, increase the time every rejection
      // This is a pretty strict linear relationship, the current design. The main goal being if
      // you've rejected multiple times players probably don't care so shouldn't get high enough to matter
      float totalWaitTime = _TimesRejected == 1 ? _TimesRejected * RejectRepeatPenaltyMaxTime_Sec :
                                                  _TimesRejected * RejectRepeatPenaltyMaxTime_Sec * ProgressiveMultiplier;
      if (Time.time < _StartTimeWait + totalWaitTime) {
        return 0.0f;
      }
      // we're off cooldown, reset accordingly
      _StartTimeWait = -1.0f;
    }
    // Flat Score, can be weighted if we have a new game or something.
    float score = BaseWeight;
    // bonus percent if it's a daily goal
    if (isChallengeDailyGoal) {
      score *= DailyGoalIncludedBonusWeight;
    }
    return score;
  }
}
