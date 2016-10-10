using UnityEngine;
using System.Collections;
using Anki.Cozmo;

namespace Cozmo.RequestGame {
  public class RequestGameListConfig : ScriptableObject {
    public RequestGameConfig[] RequestList;

    [SerializeField, Range(0f, 1f)]
    private float _WantToPlayMinigameMin_percent = 0.3f;
    public float WantToPlayMin {
      get { return _WantToPlayMinigameMin_percent; }
    }

    [SerializeField, Range(0f, 1f)]
    private float _WantToPlayMinigameMax_percent = 1.0f;
    public float WantToPlayMax {
      get { return _WantToPlayMinigameMax_percent; }
    }

    [SerializeField]
    private AnimationCurve _WantToPlayCurveBasedOnDailyGoalProgress;
    public AnimationCurve WantToPlayCurve {
      get { return _WantToPlayCurveBasedOnDailyGoalProgress; }
    }

    private static RequestGameListConfig _sInstance;

    public static void SetInstance(RequestGameListConfig instance) {
      _sInstance = instance;
    }

    public static RequestGameListConfig Instance {
      get { return _sInstance; }
    }
  }
}