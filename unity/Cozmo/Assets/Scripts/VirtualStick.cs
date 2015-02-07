using UnityEngine;
using System.Collections;
using UnityEngine.EventSystems;

public class VirtualStick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler {

	[SerializeField] bool dynamic = true; //recenter the stick from touchdown position
	[SerializeField] RectTransform bg = null;
	[SerializeField] RectTransform stick = null;
	[SerializeField] Vector2 deadZone = new Vector2(0.01f, 0.01f);
	[SerializeField] float widthInches = 1.5f;
	[SerializeField] float heightInches = 1.5f;

	[SerializeField] bool horizontalOnly = false;
	[SerializeField] bool verticalOnly = false;

	float radius;
	Canvas canvas;
	RectTransform rTrans;
	ScreenOrientation orientation;

	public Vector2 StickCenterOnScreen {
		get {
			return ZoneCenter + bg.anchoredPosition;
		}
	}

	public Vector2 ZoneCenter {
		get {
			return (Vector2)rTrans.position + rTrans.rect.center;
		}
	}

	public Vector2 JoystickData {
		get {
			Vector2 res = stick.anchoredPosition;
			return res / radius;
		}
	}

	public float Horizontal {
		get { 
			float x = JoystickData.x;
			if(Mathf.Abs(x) < deadZone.x)
				return 0f;
			return x; 
		}
	}

	public float Vertical {
		get { 
			float y = JoystickData.y;
			if(Mathf.Abs(y) < deadZone.y)
				return 0f;
			return y; 
		}
	}

	void Awake() {
		canvas = GetComponentInParent<Canvas>();
		rTrans = transform as RectTransform;

		ResizeStickToInches();

		bg.anchoredPosition = Vector3.zero;

		if(verticalOnly) {
			radius = bg.sizeDelta.y * 0.5f;
		}
		else if(horizontalOnly) {
			radius = bg.sizeDelta.x * 0.5f;
		}
		else {
			radius = Mathf.Min(bg.sizeDelta.x, bg.sizeDelta.y) * 0.5f;
		}

		//Debug.Log ("Joystick Awake startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
		bg.gameObject.SetActive(!dynamic);
		stick.gameObject.SetActive(!dynamic);
	}

	void ResizeStickToInches() {
		if(Screen.dpi == 0f)
			return;

		Vector3 size = bg.sizeDelta;
		size.x = Screen.dpi * widthInches;
		size.y = Screen.dpi * heightInches;
		bg.sizeDelta = size;
		bg.anchoredPosition = Vector3.zero;
		stick.anchoredPosition = Vector3.zero;

		if(verticalOnly) {
			radius = bg.sizeDelta.y * 0.5f;
		}
		else if(horizontalOnly) {
			radius = bg.sizeDelta.x * 0.5f;
		}
		else {
			radius = Mathf.Min(bg.sizeDelta.x, bg.sizeDelta.y) * 0.5f;
		}
	}

	void Update() {
		if(Screen.orientation != orientation) {
			bg.anchoredPosition = Vector3.zero;
			stick.anchoredPosition = Vector3.zero;
			orientation = Screen.orientation;
		}
	}

	public void OnPointerDown(PointerEventData eventData) {

		if(dynamic) {
			bg.anchoredPosition = eventData.position - ZoneCenter;
			stick.anchoredPosition = Vector3.zero;
		}

		bg.gameObject.SetActive(true);
		stick.gameObject.SetActive(true);

		ProcessStick(eventData);
		//Debug.Log ("Joystick OnPointerDown eventData.position(" + eventData.position + ") ZoneCenter("+ZoneCenter+")");
	}

	public void OnDrag(PointerEventData eventData) {
		ProcessStick(eventData);
	}

	public void OnPointerUp(PointerEventData eventData) {
		bg.anchoredPosition = Vector3.zero;
		stick.anchoredPosition = Vector3.zero;

		bg.gameObject.SetActive(!dynamic);
		stick.gameObject.SetActive(!dynamic);
		//Debug.Log ("Joystick OnPointerUp startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
	}

	void ProcessStick(PointerEventData evnt) {
		Vector2 delta = Vector2.zero;
		delta = evnt.position - StickCenterOnScreen;
		delta = Vector3.ClampMagnitude(delta, radius);

		if(verticalOnly) {
			delta.x = 0f;
		}
		if(horizontalOnly) {
			delta.y = 0f;
		}

		stick.anchoredPosition = delta;
		//Debug.Log ("Joystick renderMode(" + canvas.renderMode + ") evnt.position(" + evnt.position + ") delta(" + delta + ") bg.anchoredPosition("+bg.anchoredPosition+") stick.anchoredPosition("+stick.anchoredPosition+")");
	}
}
