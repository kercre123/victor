using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Anki.Cozmo.Viz {
  public class VizEmotionMonitor : MonoBehaviour {

    [SerializeField]
    private RectTransform _BarTray;

    [SerializeField]
    private VizEmotionBar _BarPrefab;

    private List<VizEmotionBar> _EmotionBars = new List<VizEmotionBar>();

    private void Awake() {
      for(int i = 0; i < (int)EmotionType.Count; i++) {
        var bar = UIManager.CreateUIElement(_BarPrefab, _BarTray).GetComponent<VizEmotionBar>();
        _EmotionBars.Add(bar);
        bar.SetLabel(((EmotionType)i).ToString());
      }
    }
    
    // Update is called once per frame
    private void Update () {
      var emotions = VizManager.Instance.Emotions;

      if (emotions != null) {
        for (int i = 0; i < emotions.Length; i++) {
          _EmotionBars[i].SetValue(emotions[i]);  
        }
      }
    }
  }
}