using UnityEngine;
using System.Collections;

namespace Cozmo.HomeHub {
  public class TabPanel : MonoBehaviour {

    private HomeView _HomeViewInstance;

    public virtual void Initialize(HomeView homeViewInstance) {
      _HomeViewInstance = homeViewInstance;
    }

    public HomeView GetHomeViewInstance() {
      return _HomeViewInstance;
    }
  }
}