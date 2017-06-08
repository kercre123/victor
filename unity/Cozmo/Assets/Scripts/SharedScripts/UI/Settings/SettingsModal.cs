using Cozmo.RequestGame;
using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;

namespace Cozmo.UI {
  public class SettingsModal : BaseModal {
    public static ModalPriorityData SettingsSubModalPriorityData() {
      return new ModalPriorityData(ModalPriorityLayer.Low, 1,
                                   LowPriorityModalAction.CancelSelf, HighPriorityModalAction.Stack);
    }

    private const int kCubesCount = 3;
    private const int kCubesHelpIndex = 1;

    [SerializeField]
    private ScrollRect _SettingsScrollRect;

    [SerializeField]
    private TabPanel[] _PanelPrefabs;

    [SerializeField]
    private SnappableLayoutGroup _SnappableLayoutGroup;

    [SerializeField]
    private ParentLayoutContentSizeFitter _ParentLayoutContentSizeFitter;

    [SerializeField]
    private HorizontalLayoutGroup _HorizontalLayoutGroup;

    private List<TabPanel> _TabPanelsList = new List<TabPanel>();

    public void Initialize(BaseView homeViewInstance) {

      //float totalWidth = 0f;
      for (int i = 0; i < _PanelPrefabs.Length; ++i) {
        TabPanel newTabPanel = UIManager.CreateUIElement(_PanelPrefabs[i].gameObject, _SettingsScrollRect.content).GetComponent<TabPanel>();
        newTabPanel.Initialize(homeViewInstance);
        _TabPanelsList.Add(newTabPanel);
        _SnappableLayoutGroup.RegisterLayoutElement(newTabPanel.LayoutElement);
      }

      RequestGameManager.Instance.DisableRequestGameBehaviorGroups();

      if (RobotEngineManager.Instance.CurrentRobot != null && RobotEngineManager.Instance.CurrentRobot.LightCubes.Count != kCubesCount) {
        ScrollToCubeSettings();
      }
    }

    public float GetNormalizedSnapIndexPosition(int index) {
      return _SnappableLayoutGroup.GetNormalizedSnapValue(index);
    }

    void OnDestroy() {
      RequestGameManager.Instance.EnableRequestGameBehaviorGroups();
    }

    public void ScrollToCubeSettings() {
      float x = GetNormalizedSnapIndexPosition(kCubesHelpIndex);
      _SettingsScrollRect.horizontalNormalizedPosition = x;
    }
  }
}