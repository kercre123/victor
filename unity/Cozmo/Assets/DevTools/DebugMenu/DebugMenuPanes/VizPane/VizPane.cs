using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class VizPane : MonoBehaviour {

    [SerializeField]
    private Text _MemoryMapToggleText;
    public Button _MemoryMapToggleButton;

    void Start() {
      _MemoryMapToggleButton.onClick.AddListener(ToggleMemoryMap);
      _MemoryMapToggleText.text = string.Format("Toggle Memory Map Viz : {0}", VizManager.Instance.RenderMemoryMap);
    }

    void OnDestroy() {
      _MemoryMapToggleButton.onClick.RemoveAllListeners();
    }

    public void ToggleMemoryMap() {
      VizManager.Instance.RenderMemoryMap = !VizManager.Instance.RenderMemoryMap;
      _MemoryMapToggleText.text = string.Format("Toggle Memory Map Viz : {0}", VizManager.Instance.RenderMemoryMap);
    }
  }
}