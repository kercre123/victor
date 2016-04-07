using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo.HomeHub {
  public class HomeViewTab : MonoBehaviour {

    [SerializeField]
    private TabPanel[] _TabViewPanelPrefabs;

    private List<TabPanel> _TabPanelsList = new List<TabPanel>();

    public void Initialize(HomeView homeViewInstance) {

      for (int i = 0; i < _TabViewPanelPrefabs.Length; ++i) {
        TabPanel newTabPanel = UIManager.CreateUIElement(_TabViewPanelPrefabs[i].gameObject, this.transform).GetComponent<TabPanel>();
        newTabPanel.Initialize(homeViewInstance);
        _TabPanelsList.Add(newTabPanel);
      }

    }

  }
}