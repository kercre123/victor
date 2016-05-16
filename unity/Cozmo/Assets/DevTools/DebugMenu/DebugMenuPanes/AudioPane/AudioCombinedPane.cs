using UnityEngine;
using System.Collections.Generic;
using UnityEngine.UI;

public class AudioCombinedPane : MonoBehaviour {

  [System.Serializable]
  public struct TabOption {
    public GameObject Prefab;
    public string Name;
  }

  [SerializeField]
  private List<TabOption> _Options = new List<TabOption>();

  [SerializeField]
  private ToggleGroup _ToggleGroup;

  [SerializeField]
  private AudioCombinedTab _TabPrefab;

  [SerializeField]
  private RectTransform _PanelTray;

  private GameObject _CurrentTab;

  private void Awake() {
    for (int i = 0; i < _Options.Count; i++) {
      CreateTabForOption(_Options[i], i == 0);
    }
  }

  private void CreateTabForOption(TabOption option, bool selected) {
    var tab = UIManager.CreateUIElement(_TabPrefab, _ToggleGroup.transform).GetComponent<AudioCombinedTab>();

    tab.Text = option.Name;
    tab.SetToggleGroup(_ToggleGroup);
      
    tab.OnSelect += () => {
      if(_CurrentTab != null) {
        Destroy(_CurrentTab);
      }

      _CurrentTab = UIManager.CreateUIElement(option.Prefab, _PanelTray);
    };


    if (selected) {
      tab.Select();
    }
  }
}
