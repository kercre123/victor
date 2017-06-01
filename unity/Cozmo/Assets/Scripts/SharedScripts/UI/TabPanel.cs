using UnityEngine;
using System.Collections;

namespace Cozmo.UI {
  public class TabPanel : MonoBehaviour {

    protected BaseView _BaseViewInstance;

    public UnityEngine.UI.LayoutElement LayoutElement {
      get { return _LayoutElement; }
    }

    [SerializeField]
    private UnityEngine.UI.LayoutElement _LayoutElement;

    public virtual void Initialize(BaseView homeViewInstance) {
      _BaseViewInstance = homeViewInstance;
    }

    public BaseView GetBaseViewInstance() {
      return _BaseViewInstance;
    }
  }
}