using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.HomeHub {
  public class HomeViewTab : MonoBehaviour {

    [SerializeField]
    private TabPanel[] _TabViewPanelPrefabs;

    [SerializeField]
    private bool _DisableGameRequestsWhenOpen;

    private List<TabPanel> _TabPanelsList = new List<TabPanel>();

    private Transform _Container;

    private float _TotalPanelsWidth;

    public void Initialize(HomeView homeViewInstance) {
      if (_Container == null) {
        _Container = transform;
      }
      for (int i = 0; i < _TabViewPanelPrefabs.Length; ++i) {
        TabPanel newTabPanel = UIManager.CreateUIElement(_TabViewPanelPrefabs[i].gameObject, _Container).GetComponent<TabPanel>();
        newTabPanel.Initialize(homeViewInstance);
        _TabPanelsList.Add(newTabPanel);
        _TotalPanelsWidth += _TabViewPanelPrefabs[i].GetComponent<UnityEngine.UI.LayoutElement>().minWidth;
      }
      EnableGameRequestsIfAllowed(!_DisableGameRequestsWhenOpen);
    }

    public float GoToIndex(int index) {
      if (index > _TabPanelsList.Count) {
        DAS.Error("HomeViewTab.GoToIndex out of bounds", gameObject.name);
        return 0.0f;
      }

      float indexedValue = 0.0f;
      for (int i = 0; i < _TabPanelsList.Count; ++i) {
        if (i < index) {
          indexedValue += _TabPanelsList[i].GetComponent<UnityEngine.UI.LayoutElement>().minWidth;
        }
        else {
          break;
        }
      }

      float scrollNormalizedPosition = (indexedValue + _TabPanelsList[index].GetComponent<UnityEngine.UI.LayoutElement>().minWidth / 2.0f) / _TotalPanelsWidth;

      return scrollNormalizedPosition;
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