using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public class LightCountNoColorSpeedTapRules : SpeedTapRulesBase {

    public override void SetLights(bool shouldTap, SpeedTapGame game) {

      if (shouldTap) {
        // Do match
        int lightCount = UnityEngine.Random.Range(1, 5);
        game.CozmoWinColor = game.PlayerWinColor = _Colors[0];
        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);
        for (int i = 0; i < lightCount; ++i) {
          game.CozmoBlock.Lights[i].OnColor = _Colors[0].ToUInt();
          game.PlayerBlock.Lights[i].OnColor = _Colors[0].ToUInt();
        }

      }
      else {
        // Do non-match
        int lightCountPlayer = UnityEngine.Random.Range(1, 5);
        int lightCountCozmo = 1 + ((lightCountPlayer + UnityEngine.Random.Range(0, 3)) % 4);

        game.CozmoBlock.SetLEDs(Color.black);
        game.PlayerBlock.SetLEDs(Color.black);

        for (int i = 0; i < lightCountPlayer; ++i) {
          game.PlayerBlock.Lights[i].OnColor = _Colors[0].ToUInt();
        }

        for (int i = 0; i < lightCountCozmo; ++i) {
          game.CozmoBlock.Lights[i].OnColor = _Colors[0].ToUInt();
        }

      }
    }
  }
}