using UnityEngine;
using System.Collections;

public static class AnimationGroupName {

  #region Request Game

  public const string kRequestGame_Confirm = "requestGame_Confirm";
  public const string kRequestGame_Reject = "requestGame_Reject";

  #endregion

  #region Speed Tap

  public const string kSpeedTap_Driving_Start = "ag_speedTap_startDriving";
  public const string kSpeedTap_Driving_Loop = "ag_speedTap_drivingLoop";
  public const string kSpeedTap_Driving_End = "ag_speedTap_endDriving";

  #endregion

  #region Cube Pounce

  public const string kCubePounce_Fake = "cubePounce_Fake";
  public const string kCubePounce_Pounce = "cubePounce_Pounce";
  public const string kCubePounce_WinHand = "cubePounce_WinHand";
  public const string kCubePounce_WinRound = "cubePounce_WinRound";
  public const string kCubePounce_WinSession = "cubePounce_WinSession";
  public const string kCubePounce_LoseHand = "cubePounce_LoseHand";
  public const string kCubePounce_LoseRound = "cubePounce_LoseRound";
  public const string kCubePounce_LoseSession = "cubePounce_LoseSession";

  #endregion

  #region Simon

  public const string kSimonStartTurn = "ag_simon_cozmo_end_listen";

  #endregion
}
