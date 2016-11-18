using UnityEngine;
using System.Collections;

namespace Cozmo.HomeHub {
  public class TabPanel : MonoBehaviour {

    private HomeView _HomeViewInstance;

    [SerializeField]
    private UnityEngine.UI.LayoutElement _LayoutElement;

    public virtual void Initialize(HomeView homeViewInstance) {
      _HomeViewInstance = homeViewInstance;
    }

    public UnityEngine.UI.LayoutElement GetLayoutElement() {
      if (_LayoutElement == null) {
        Debug.LogError(gameObject.name);
      }
      return _LayoutElement;
    }

    public HomeView GetHomeViewInstance() {
      return _HomeViewInstance;
    }
  }
}