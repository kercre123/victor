using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.UI {
  public class ViewTab : MonoBehaviour {

    [SerializeField]
    private TabPanel[] _TabViewPanelPrefabs;

    [SerializeField]
    private bool _DisableGameRequestsWhenOpen;

    [SerializeField]
    private SnappableLayoutGroup _SnappableLayoutGroup;

    [SerializeField]
    private ParentLayoutContentSizeFitter _ParentLayoutContentSizeFitter;

    public ParentLayoutContentSizeFitter ParentLayoutContentSizeFitter {
      get {
        return _ParentLayoutContentSizeFitter;
      }
    }

    private List<TabPanel> _TabPanelsList = new List<TabPanel>();

    private Transform _Container;

    public void Initialize(BaseView homeViewInstance) {
      if (_Container == null) {
        _Container = transform;
      }
      for (int i = 0; i < _TabViewPanelPrefabs.Length; ++i) {
        TabPanel newTabPanel = UIManager.CreateUIElement(_TabViewPanelPrefabs[i].gameObject, _Container).GetComponent<TabPanel>();
        newTabPanel.Initialize(homeViewInstance);
        _TabPanelsList.Add(newTabPanel);
        _SnappableLayoutGroup.RegisterLayoutElement(newTabPanel.LayoutElement);
      }
      EnableGameRequestsIfAllowed(!_DisableGameRequestsWhenOpen);
    }

    public float GetNormalizedSnapIndexPosition(int index) {
      return _SnappableLayoutGroup.GetNormalizedSnapValue(index);
    }

    void OnDestroy() {
      if (_DisableGameRequestsWhenOpen) {
        RobotEngineManager.Instance.RequestGameManager.EnableRequestGameBehaviorGroups();
      }
    }

    public void EnableGameRequestsIfAllowed(bool enable) {
      if (enable && !_DisableGameRequestsWhenOpen) {
        RobotEngineManager.Instance.RequestGameManager.EnableRequestGameBehaviorGroups();
      }
      else {
        RobotEngineManager.Instance.RequestGameManager.DisableRequestGameBehaviorGroups();
      }
    }
  }
}