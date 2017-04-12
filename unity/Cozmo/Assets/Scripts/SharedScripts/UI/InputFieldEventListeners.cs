using UnityEngine;
using System.Collections;
using UnityEngine.EventSystems;

public class InputFieldEventListeners : MonoBehaviour, ISelectHandler, IDeselectHandler {

  public System.Action<BaseEventData> onSelect;
  public System.Action<BaseEventData> onDeselect;

  public void OnSelect(BaseEventData data) {
    if (onSelect != null) {
      onSelect(data);
    }
  }

  public void OnDeselect(BaseEventData data) {
    if (onDeselect != null) {
      onDeselect(data);
    }
  }
}
