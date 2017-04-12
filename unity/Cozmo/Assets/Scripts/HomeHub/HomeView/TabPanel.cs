using UnityEngine;
using System.Collections;

namespace Cozmo.HomeHub {
  public class TabPanel : MonoBehaviour {

    private HomeView _HomeViewInstance;

    public UnityEngine.UI.LayoutElement LayoutElement {
      get { return _LayoutElement; }
    }

    [SerializeField]
    private UnityEngine.UI.LayoutElement _LayoutElement;

    public virtual void Initialize(HomeView homeViewInstance) {
      _HomeViewInstance = homeViewInstance;
    }

    public HomeView GetHomeViewInstance() {
      return _HomeViewInstance;
    }
  }
}