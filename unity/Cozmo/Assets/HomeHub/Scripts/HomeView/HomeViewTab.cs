using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.HomeHub {
  public class HomeViewTab : MonoBehaviour {

    [SerializeField]
    private TabPanel[] _TabViewPanelPrefabs;

    private List<TabPanel> _TabPanelsList = new List<TabPanel>();

    [SerializeField]
    private Transform _Container;

    public void Initialize(HomeView homeViewInstance) {
      if (_Container == null) {
        _Container = transform;
      }
      for (int i = 0; i < _TabViewPanelPrefabs.Length; ++i) {
        TabPanel newTabPanel = UIManager.CreateUIElement(_TabViewPanelPrefabs[i].gameObject, _Container).GetComponent<TabPanel>();
        newTabPanel.Initialize(homeViewInstance);
        _TabPanelsList.Add(newTabPanel);
      }

    }

  }
}