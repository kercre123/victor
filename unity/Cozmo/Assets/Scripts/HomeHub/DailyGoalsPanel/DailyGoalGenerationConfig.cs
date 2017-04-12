using UnityEngine;
using System.Collections;

/// <summary>
/// Daily goal generation config. Has global information about Goal Generation that isn't specific
/// to any one goal
/// </summary>
public class DailyGoalGenerationConfig : ScriptableObject {
  public string DailyGoalFileName;
  public int MaxGoals;
  public int MinGoals;

  public string OnboardingGoalFileName;
  public Sprite DailyGoalIcon;
}
