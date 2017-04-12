using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    public class GenericRewardsConfig : ScriptableObject {

      private static GenericRewardsConfig _sInstance;

      public static void SetInstance(GenericRewardsConfig instance) {
        _sInstance = instance;
      }

      public static GenericRewardsConfig Instance {
        get {
          return _sInstance;
        }
      }

      [ItemId]
      public string EnergyID = "experience";
      [ItemId]
      public string CoinID = "TestHexItem0";
      [ItemId]
      public string SparkID = "treat";

      public int ExpPerParticleEffect = 5;
      public int BurstPerParticleHit = 10;
      public float ExpParticleMinSpread = 50.0f;
      public float ExpParticleMaxSpread = 250.0f;
      public float ExpParticleBurst = 0.25f;
      public float ExpParticleHold = 0.5f;
      public float ExpParticleLeave = 0.25f;
      public float ExpParticleStagger = 0.75f;
      public string RewardConfigFilename = "";
    }

  }
}