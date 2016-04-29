using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class VizPane : MonoBehaviour {

    [SerializeField]
    private Text _InfoLabel;
    [SerializeField]
    private Text _MemoryMapToggleText;
    public Button _MemoryMapToggleButton;

    void Start() {
      VizManager.EnableTexture = true;
      _MemoryMapToggleButton.onClick.AddListener(ToggleMemoryMap);
      _MemoryMapToggleText.text = string.Format("Toggle Memory Map Viz : {0}", VizManager.Instance.RenderMemoryMap);
    }

    void OnDestroy() {
      VizManager.EnableTexture = false;
      _MemoryMapToggleButton.onClick.RemoveAllListeners();
    }

    public void ToggleMemoryMap() {
      VizManager.Instance.RenderMemoryMap = !VizManager.Instance.RenderMemoryMap;
      _MemoryMapToggleText.text = string.Format("Toggle Memory Map Viz : {0}", VizManager.Instance.RenderMemoryMap);
    }
    
    // Update is called once per frame
    private void Update() {
      _InfoLabel.text = string.Format("Current Behavior: {0}\n" +
      "Recent Mood Events: {1}\n",
        VizManager.Instance.Behavior,
        VizManager.Instance.RecentMoodEvents != null ? string.Join(", ", VizManager.Instance.RecentMoodEvents) : "");
    }
  }
}