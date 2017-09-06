using UnityEngine;
using UnityEngine.EventSystems;

namespace Cozmo.UI {
  public class TooltipGenericBaseDataComponent : MonoBehaviour {

    [SerializeField]
    protected string[] _HeaderLocKeys;

    [SerializeField]
    protected string[] _BodyLocKeys;

    [SerializeField]
    private GameObject _HitArea;

    [SerializeField]
    protected TooltipWidget.CaretPosition _PreferredDir = TooltipWidget.CaretPosition.Left;

    [SerializeField]
    protected bool _ToolTipEnabled = true;

    [SerializeField]
    protected int _ShowBodyIndex = 0;

    [SerializeField]
    protected int _ShowHeaderIndex = 0;

    protected virtual void Start() {
      if (_HitArea == null) {
        _HitArea = gameObject;
      }
      EventTrigger trigger = _HitArea.AddComponent<EventTrigger>();
      EventTrigger.Entry entry = new EventTrigger.Entry();
      entry.eventID = EventTriggerType.PointerDown;
      entry.callback.AddListener(HandlePointerDown);
      trigger.triggers.Add(entry);
    }

    protected virtual void HandlePointerDown(BaseEventData data) {
      if (!_ToolTipEnabled) {
        return;
      }
      PointerEventData ped = data as PointerEventData;
      Vector2 localCursor = ped.position;
      RectTransformUtility.ScreenPointToLocalPointInRectangle(UIManager.GetUICanvas().GetComponent<RectTransform>(), ped.position, ped.pressEventCamera, out localCursor);
      TooltipManager.Instance.ShowToolTip(GetHeaderString(), GetBodyString(), UIManager.GetUICanvas().transform, localCursor, _PreferredDir);
    }

    protected virtual string GetHeaderString() {
      return _ShowHeaderIndex < _HeaderLocKeys.Length ? Localization.Get(_HeaderLocKeys[_ShowHeaderIndex]) : string.Empty;
    }

    protected virtual string GetBodyString() {
      return _ShowBodyIndex < _BodyLocKeys.Length ? Localization.Get(_BodyLocKeys[_ShowBodyIndex]) : string.Empty;
    }

    // public API for toggling state
    public void SetTooltipEnabled(bool isEnabled) {
      _ToolTipEnabled = isEnabled;
    }

    // Which tip to show
    public void SetShowBodyIndex(int whichTip) {
      _ShowBodyIndex = whichTip;
    }
    public void SetShowHeaderIndex(int whichTip) {
      _ShowHeaderIndex = whichTip;
    }
  }

}