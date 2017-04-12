using UnityEngine;
using UnityEngine.UI;
using Anki.UI;

namespace Cozmo.CheckInFlow.UI {
  public class CheckInFlowReward : MonoBehaviour {
    [SerializeField]
    private Image _RewardIcon;
    public Image RewardIcon {
      get { return _RewardIcon; }
    }

    [SerializeField]
    private AnkiTextLegacy _RewardCount;
    public AnkiTextLegacy RewardCount {
      get { return _RewardCount; }
    }

    private float _Radius_px;
    public float Radius_px {
      get { return _Radius_px; }
    }

    public float Alpha {
      get { return _RewardIcon.color.a; }
      set {
        Color color = _RewardIcon.color;
        color.a = value;
        _RewardIcon.color = color;
      }
    }

    [HideInInspector]
    public Vector3? TargetSpawnRestPosition = null;

    private void Awake() {
      _Radius_px = ((RectTransform)this.transform).rect.width * 0.5f;
    }
  }
}