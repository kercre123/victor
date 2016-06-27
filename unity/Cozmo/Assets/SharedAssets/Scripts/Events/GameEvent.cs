// Used for GameLogic, NOT animationEvents
namespace Anki {

namespace Cozmo {

public enum GameEvent : byte
{
  OnDriveStart,  // 0
  OnDriveEnd,  // 1
  OnFidget,  // 2
  OnSpeedtapStarted,  // 3
  OnSpeedtapHandCozmoWin,  // 4
  OnSpeedtapHandPlayerWin,  // 5
  OnSpeedtapRoundCozmoWinAnyIntensity,  // 6
  OnSpeedtapRoundCozmoWinLowIntensity,  // 7
  OnSpeedtapRoundCozmoWinHighIntensity,  // 8
  OnSpeedtapRoundPlayerWinAnyIntensity,  // 9
  OnSpeedtapRoundPlayerWinLowIntensity,  // 10
  OnSpeedtapRoundPlayerWinHighIntensity,  // 11
  OnSpeedtapGameCozmoWinAnyIntensity,  // 12
  OnSpeedtapGameCozmoWinLowIntensity,  // 13
  OnSpeedtapGameCozmoWinHighIntensity,  // 14
  OnSpeedtapGamePlayerWinAnyIntensity,  // 15
  OnSpeedtapGamePlayerWinLowIntensity,  // 16
  OnSpeedtapGamePlayerWinHighIntensity,  // 17
  OnSpeedtapGetOut,  // 18
  OnSpeedtapCozmoConfirm,  // 19
  OnSpeedtapTap,  // 20
  OnSpeedtapFakeout,  // 21
  OnSpeedtapIdle,  // 22
  OnLearnedPlayerName,  // 23
  OnSawNewUnnamedFace,  // 24
  OnSawNewNamedFace,  // 25
  OnSawOldUnnamedFace,  // 26
  OnSawOldNamedFace,  // 27
  OnWiggle,  // 28
  OnWaitForCubesMinigameSetup,  // 29
  OnSimonSetupComplete,  // 30
  OnSimonExampleStarted,  // 31
  OnSimonCozmoHandComplete,  // 32
  OnSimonPlayerHandComplete,  // 33
  OnSimonPointCube,  // 34
  OnSimonCozmoWin,  // 35
  OnSimonPlayerWin,  // 36
  OnDailyGoalCompleted,  // 37
  OnUnlockableEarned,  // 38
  OnGameComplete,  // 39
  OnMeetNewPerson,  // 40
  Count  // 41
};

} // namespace Cozmo

} // namespace Anki

