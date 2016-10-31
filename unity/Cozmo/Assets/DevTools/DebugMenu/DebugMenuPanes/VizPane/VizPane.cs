using UnityEngine;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class VizPane : MonoBehaviour {

    [SerializeField]
    private Text _MemoryMapToggleText;
    public Button _MemoryMapToggleButton;

    [SerializeField]
    private Toggle _PetReticlesEnabledToggle;

    void Start() {
      VizManager.Enabled = true;
      _MemoryMapToggleButton.onClick.AddListener(ToggleMemoryMap);
      _MemoryMapToggleText.text = string.Format("Toggle Memory Map Viz : {0}", VizManager.Instance.RenderMemoryMap);

      _PetReticlesEnabledToggle.isOn = DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.DroneModePetReticlesEnabled;
      _PetReticlesEnabledToggle.onValueChanged.AddListener(HandleTogglePetReticles);
    }

    void OnDestroy() {
      VizManager.Enabled = false;
      _MemoryMapToggleButton.onClick.RemoveAllListeners();
      _PetReticlesEnabledToggle.onValueChanged.RemoveListener(HandleTogglePetReticles);
    }

    public void ToggleMemoryMap() {
      VizManager.Instance.RenderMemoryMap = !VizManager.Instance.RenderMemoryMap;
      _MemoryMapToggleText.text = string.Format("Toggle Memory Map Viz : {0}", VizManager.Instance.RenderMemoryMap);
    }

    private void HandleTogglePetReticles(bool enable) {
      DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.DroneModePetReticlesEnabled = enable;
      DataPersistence.DataPersistenceManager.Instance.Save();
    }
  }
}