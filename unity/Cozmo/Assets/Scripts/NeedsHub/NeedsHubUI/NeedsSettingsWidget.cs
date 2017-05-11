using Cozmo.HomeHub;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Needs.UI {
  public class NeedsSettingsWidget : MonoBehaviour {
    [SerializeField]
    private ScrollRect _SettingsScrollRect;

    [SerializeField]
    private GameObject _SettingsContainer;

    [SerializeField]
    private HomeViewTab _SettingsTabPrefab;
    private HomeViewTab _SettingsTabInstance;

    public void ShowSettings() {
      _SettingsScrollRect.horizontalNormalizedPosition = 0.0f;

      if (_SettingsTabInstance == null) {
        _SettingsTabInstance = Instantiate(_SettingsTabPrefab.gameObject).GetComponent<HomeViewTab>();
        _SettingsTabInstance.transform.SetParent(_SettingsContainer.transform, false);

        // Since settings doesn't rely on Homethis should be safe
        _SettingsTabInstance.Initialize(null);
      }

      _SettingsTabInstance.gameObject.SetActive(true);
    }

    public void HideSettings() {
      if (_SettingsTabInstance != null) {
        _SettingsTabInstance.gameObject.SetActive(false);
      }
    }

    public void ScrollToCubeSettings() {
      _SettingsTabInstance.ParentLayoutContentSizeFitter.OnResizedParent += () => {
        const int kCubesHelpIndex = 1;
        _SettingsScrollRect.horizontalNormalizedPosition = _SettingsTabInstance.GetNormalizedSnapIndexPosition(kCubesHelpIndex);
      };
    }
  }
}