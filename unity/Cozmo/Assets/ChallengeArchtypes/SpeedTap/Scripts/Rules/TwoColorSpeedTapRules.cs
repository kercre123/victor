using UnityEngine;
using System.Collections;

namespace SpeedTap {
  /// <summary>
  /// Show one-two colors on each cube in a ABAB pattern.
  /// Colors on the same cube are allowed to match.
  /// When player's cube and cozmo's are meant to NOT match make
  /// sure NONE are the colors are common between the two cubes.
  /// </summary>
  public class TwoColorSpeedTapRules : SpeedTapRulesBase {

    public override void SetLights(bool shouldMatch, SpeedTapGame game) {
      if (shouldMatch) {
        // Do match
        // Pick two base colors; they can be the same.
        // By design / Sean: randColorIndex and randColorIndex2 are allowed to match
        int randColorIndex = PickRandomColor();
        int randColorIndex2 = PickRandomColor();

        SetABABLights(game.PlayerWinColors, randColorIndex, randColorIndex2);
        SetABABLights(game.CozmoWinColors, randColorIndex, randColorIndex2);

        game.SetLEDs(game.PlayerBlockID, game.PlayerWinColors);
        game.SetLEDs(game.CozmoBlockID, game.CozmoWinColors);
      }
      else {
        // Do non-match
        if (!TrySetCubesRed(game)) {
          // Pick different base colors for the player and Cozmo
          int playerColorIdx, cozmoColorIdx;
          PickTwoDifferentColors(out playerColorIdx, out cozmoColorIdx);

          // The player's color should not match cozmo's at all; can match their own
          int playerColorIdx2 = PickRandomColor();
          while (playerColorIdx2 == cozmoColorIdx) {
            playerColorIdx2 = PickRandomColor();
          }

          // Cozmo's color should not match the player's at all; can match their own
          int cozmoColorIdx2 = PickRandomColor();
          while (cozmoColorIdx2 == playerColorIdx || cozmoColorIdx == playerColorIdx2) {
            cozmoColorIdx2 = PickRandomColor();
          }

          SetABABLights(game.PlayerWinColors, playerColorIdx, playerColorIdx2);
          SetABABLights(game.CozmoWinColors, cozmoColorIdx, cozmoColorIdx2);

          game.SetLEDs(game.PlayerBlockID, game.PlayerWinColors);
          game.SetLEDs(game.CozmoBlockID, game.CozmoWinColors);
        }
      }
    }
  }
}