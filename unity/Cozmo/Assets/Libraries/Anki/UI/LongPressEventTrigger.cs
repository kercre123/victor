using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using System.Collections;

// based on source from Unity forums:
// http://forum.unity3d.com/threads/long-press-gesture-on-ugui-button.264388/#post-1911939

public class LongPressEventTrigger : UIBehaviour, IPointerDownHandler, IPointerUpHandler, IPointerExitHandler {
  [ Tooltip( "How long must pointer be down on this object to trigger a long press" ) ]
  public float
    durationThreshold = 0.5f; // 0.5 sec is the default long press duration on iOS
	
  public UnityEvent onLongPress = new UnityEvent();
	
  private bool isPointerDown = false;
  private bool longPressTriggered = false;
  private float timePressStarted;
	
	
  private void Update() {
    if (isPointerDown && !longPressTriggered) {
      if (Time.time - timePressStarted > durationThreshold) {
        longPressTriggered = true;
        onLongPress.Invoke();
      }
    }
  }
	
  public void OnPointerDown(PointerEventData eventData) {
    timePressStarted = Time.time;
    isPointerDown = true;
    longPressTriggered = false;
  }
	
  public void OnPointerUp(PointerEventData eventData) {
    isPointerDown = false;
  }
	
  public void OnPointerExit(PointerEventData eventData) {
    isPointerDown = false;
  }
}