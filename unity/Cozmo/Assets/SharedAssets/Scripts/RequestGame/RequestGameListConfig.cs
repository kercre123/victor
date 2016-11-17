using UnityEngine;

namespace Cozmo.RequestGame {
  public class RequestGameListConfig : ScriptableObject {
    public RequestGameConfig[] RequestList;

    private static RequestGameListConfig _sInstance;

    public static void SetInstance(RequestGameListConfig instance) {
      _sInstance = instance;
    }

    public static RequestGameListConfig Instance {
      get { return _sInstance; }
    }
  }
}