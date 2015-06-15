using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;

public class DynamicSliderFrame : MonoBehaviour, IPointerDownHandler, IPointerUpHandler, IDragHandler {
	[SerializeField] Slider slider;
	[SerializeField] RectTransform hintsToHide;
	[SerializeField] Text description;

	RectTransform rTrans;
	RectTransform sliderTrans;
	EventTrigger sliderTrigger;

	Canvas canvas;

	bool dynamic = true;

	void Awake() {
		rTrans = transform as RectTransform;
		sliderTrans = slider.transform as RectTransform;
		sliderTrigger = slider.gameObject.GetComponent<EventTrigger>();

		canvas = GetComponentInParent<Canvas>();
	}
	
	void OnEnable() {
		RefreshToScreenSettings();

		ScreenMultiSettingsDetector.ShareSettings += RefreshToScreenSettings;

		if(hintsToHide != null) hintsToHide.gameObject.SetActive(dynamic);
		if(description != null) description.gameObject.SetActive(true);
		if(sliderTrans != null) sliderTrans.gameObject.SetActive(!dynamic);
	}

	void OnDisable() {
		if(sliderTrans != null) sliderTrans.gameObject.SetActive(false);
		if(hintsToHide != null) hintsToHide.gameObject.SetActive(false);
		if(description != null) description.gameObject.SetActive(false);
		ScreenMultiSettingsDetector.ShareSettings -= RefreshToScreenSettings;
	}

	public void OnPointerDown(PointerEventData eventData) {
		if(dynamic) {
			Vector2 localPointerPos;
			RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, eventData.position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out localPointerPos);
			sliderTrans.gameObject.SetActive(true);
			localPointerPos.x += rTrans.sizeDelta.x * 0.5f;
			localPointerPos.y -= rTrans.sizeDelta.y * 0.5f;
			sliderTrans.anchoredPosition = localPointerPos;

			//Debug.Log("DynamicSliderFrame sliderTrans.anchoredPosition("+sliderTrans.anchoredPosition+") eventData.position("+eventData.position+") localPointerPos("+localPointerPos+")");
			if(hintsToHide != null) {
				hintsToHide.gameObject.SetActive(false);
			}
			slider.OnPointerDown(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerDown(eventData);
		}
		else if(RectTransformUtility.RectangleContainsScreenPoint(sliderTrans, eventData.position, canvas.GetComponent<Camera>())) {
			slider.OnPointerDown(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerDown(eventData);
		}
	}

	public void OnPointerUp(PointerEventData eventData) {

		if(dynamic) {
			slider.OnPointerUp(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerUp(eventData);
			sliderTrans.gameObject.SetActive(false);

			if(hintsToHide != null) hintsToHide.gameObject.SetActive(true);
		}
		else if(RectTransformUtility.RectangleContainsScreenPoint(sliderTrans, eventData.position, canvas.GetComponent<Camera>())) {
			slider.OnPointerUp(eventData);
			if(sliderTrigger != null) sliderTrigger.OnPointerUp(eventData);
		}
	}

	public void OnDrag(PointerEventData eventData) {
		if(dynamic) {
			slider.OnDrag(eventData);
			if(sliderTrigger != null) sliderTrigger.OnDrag(eventData);
		}
		else if(RectTransformUtility.RectangleContainsScreenPoint(sliderTrans, eventData.position, canvas.GetComponent<Camera>())) {
			slider.OnDrag(eventData);
			if(sliderTrigger != null) sliderTrigger.OnDrag(eventData);
		}
	}

	void RefreshToScreenSettings() {
		dynamic = ScreenMultiSettingsDetector.CurrentIndex == 0;
		//Debug.Log(gameObject.name + " RefreshToScreenSettings dynamic("+dynamic+")" );
	}
}
