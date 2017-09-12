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
      Vector2 borderSize = TooltipManager.Instance.MinBorderSpaceToEdge;
      Vector2 clampedPos = new Vector2(Mathf.Clamp(ped.position.x, borderSize.x, Screen.width - borderSize.x),
                                      Mathf.Clamp(ped.position.y, borderSize.y, Screen.height - borderSize.y));
      Vector2 localCursor = new Vector2(clampedPos.x, clampedPos.y);
      RectTransformUtility.ScreenPointToLocalPointInRectangle(UIManager.GetUICanvas().GetComponent<RectTransform>(), clampedPos, ped.pressEventCamera, out localCursor);
      DAS.Event("tooltip.show", GetBodyLocKey());
      TooltipManager.Instance.ShowToolTip(GetHeaderString(), GetBodyString(), UIManager.GetUICanvas().transform, localCursor, _PreferredDir);
    }

    // Base classes should override if need formatting
    protected virtual string GetHeaderString() {
      string headerLocKey = GetHeaderLocKey();
      return headerLocKey == string.Empty ? string.Empty : Localization.Get(headerLocKey);
    }

    protected virtual string GetBodyString() {
      string bodyLocKey = GetBodyLocKey();
      return bodyLocKey == string.Empty ? string.Empty : Localization.Get(bodyLocKey);
    }

    protected virtual string GetHeaderLocKey() {
      return _ShowHeaderIndex < _HeaderLocKeys.Length ? _HeaderLocKeys[_ShowHeaderIndex] : string.Empty;
    }

    protected virtual string GetBodyLocKey() {
      return _ShowBodyIndex < _BodyLocKeys.Length ? _BodyLocKeys[_ShowBodyIndex] : string.Empty;
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