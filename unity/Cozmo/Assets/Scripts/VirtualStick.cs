using UnityEngine;
using System.Collections;
using UnityEngine.EventSystems;

public class VirtualStick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler {
	[SerializeField] bool dynamic = true; //recenter the stick from touchdown position
	[SerializeField] RectTransform bg = null;
	[SerializeField] RectTransform stick = null;
	[SerializeField] Vector2 deadZone = new Vector2(0.01f, 0.01f);

	[SerializeField] bool clearHorizontal = true;
	[SerializeField] bool clearVertical = true;

	[SerializeField] float widthInches = 1.5f;
	[SerializeField] float heightInches = 1.5f;
	[SerializeField] bool resizeStickNotBG = false;

	[SerializeField] bool allowAxisSwipes = false;
	[SerializeField] bool allowPreciseSwipes = false;

	[SerializeField] float swipeMinSpeed = 4f;

	[SerializeField] bool horizontalOnly = false;
	[SerializeField] bool verticalOnly = false;

	float radius;
	//Canvas canvas;
	RectTransform rTrans;
	ScreenOrientation orientation;

	bool pressed = false;
	float pressedTime = 0f;

	Vector2 touchDownValue = Vector2.zero;
	Vector2 swipedDirection = Vector2.zero;
	bool disallowSwipeForThisTouch = false;

	public bool IsPressed { get { return pressed; } }
	public float PressedTime { get { return pressedTime; } }
	 
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
			if(horizontalOnly) {
				res.y = 0f;
			}

			if(verticalOnly) {
				res.x = 0f;
			}

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
		//canvas = GetComponentInParent<Canvas>();
		rTrans = transform as RectTransform;
	}

	void OnEnable() {
		//canvas = GetComponentInParent<Canvas>();
		rTrans = transform as RectTransform;

		ResizeStickToInches();

		bg.anchoredPosition = Vector3.zero;
		RefreshRadius();

		//Debug.Log ("Joystick Awake startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
		bg.gameObject.SetActive(!dynamic);
		stick.gameObject.SetActive(!dynamic);
	}

	void RefreshRadius() {
		if(verticalOnly) {
			radius = bg.rect.height * 0.5f;
		}
		else if(horizontalOnly) {
			radius = bg.rect.width * 0.5f;
		}
		else {
			radius = Mathf.Min(bg.rect.width, bg.rect.height) * 0.5f;
		}
	}

	void ResizeStickToInches() {
		if(Screen.dpi == 0f)
			return;

		RectTransform rectT = bg;
		if(resizeStickNotBG)
			rectT = stick;

		Vector3 size = rectT.sizeDelta;
		size.x = Screen.dpi * widthInches;
		size.y = Screen.dpi * heightInches;
		rectT.sizeDelta = size;
		rectT.anchoredPosition = Vector3.zero;
		stick.anchoredPosition = Vector3.zero;

		RefreshRadius();
	}

	void Update() {
		if(Screen.orientation != orientation) {
			bg.anchoredPosition = Vector3.zero;
			stick.anchoredPosition = Vector3.zero;
			orientation = Screen.orientation;
		}

		if(pressed) {
			pressedTime += Time.deltaTime;
		}
		else {
			pressedTime = 0f;

			//if we've allowed a swipe to override stick, then snap it here
			if(swipedDirection.sqrMagnitude > 0f) {
				Vector3 anchorPos = swipedDirection * radius;
				stick.anchoredPosition = anchorPos;
			}
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
		pressed = true;
		pressedTime = 0f;

		//if this is first touch after a swipe, prevent swipe input for this touch
		disallowSwipeForThisTouch = swipedDirection.sqrMagnitude > 0f;
		swipedDirection = Vector2.zero;
		touchDownValue = JoystickData;

		//Debug.Log ("Joystick OnPointerDown eventData.position(" + eventData.position + ") ZoneCenter("+ZoneCenter+") disallowSwipeForThisTouch("+disallowSwipeForThisTouch+")");
	}

	public void OnDrag(PointerEventData eventData) {
		ProcessStick(eventData);
	}

	public void OnPointerUp(PointerEventData eventData) {
		ProcessStick(eventData);
		ConsiderSwipes(eventData.position);

		if(dynamic) bg.anchoredPosition = Vector3.zero;

		if(clearHorizontal) {
			Vector3 anchor = stick.anchoredPosition;
			anchor.x = 0f;
			stick.anchoredPosition = anchor;
		}

		if(clearVertical) {
			Vector3 anchor = stick.anchoredPosition;
			anchor.y = 0f;
			stick.anchoredPosition = anchor;
		}

		pressed = false;
		pressedTime = 0f;
		bool swiped = swipedDirection.sqrMagnitude > 0f;
		bg.gameObject.SetActive(!dynamic || swiped);
		stick.gameObject.SetActive(!dynamic || swiped);
		//Debug.Log ("Joystick OnPointerUp StickCenterOnScreen(" + StickCenterOnScreen + ") stick.anchoredPosition("+stick.anchoredPosition+")");
	}

	void ConsiderSwipes(Vector2 pos) {
		Vector3 oldSwipe = swipedDirection;
		swipedDirection = Vector2.zero;
		if(disallowSwipeForThisTouch) return;
		if(!allowAxisSwipes && !allowPreciseSwipes) return;

		if(pressedTime <= 0f) return;
		if(pressedTime > 1f) return;

		Vector2 delta = JoystickData - touchDownValue;
		float distance = delta.magnitude;

		if(distance < swipeMinSpeed * pressedTime)
			return;

		swipedDirection = delta.normalized;

		if(oldSwipe.sqrMagnitude > 0f && Vector2.Dot(oldSwipe, swipedDirection) < 0f) {
			swipedDirection = Vector2.zero;
		}

		if(allowPreciseSwipes && !horizontalOnly && !verticalOnly) return;

		float dotUp = Vector2.Dot(swipedDirection, Vector2.up);

		if(verticalOnly) {
			if(dotUp >= 0f) {
				swipedDirection = Vector2.up;
			}
			else {
				swipedDirection = -Vector2.up;
			}
			return;
		}

		float dotRight = Vector2.Dot(swipedDirection, Vector2.right);

		if(horizontalOnly) {
			if(dotRight >= 0f) {
				swipedDirection = Vector2.right;
			}
			else {
				swipedDirection = -Vector2.right;
			}
			return;
		}

		if(Mathf.Abs(dotUp) >= Mathf.Abs(dotRight)) {
			if(dotUp >= 0f) {
				swipedDirection = Vector2.up;
			}
			else { 
				swipedDirection = -Vector2.up;
			}
		}
		else if(dotRight >= 0f) {
			swipedDirection = Vector2.right;
		}
		else {
			swipedDirection = -Vector2.right;
		}
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
		//Debug.Log ("Joystick evnt.position(" + evnt.position + ") radius(" + radius + ") delta(" + delta + ") bg.anchoredPosition("+bg.anchoredPosition+") stick.anchoredPosition("+stick.anchoredPosition+")");
	}
}
