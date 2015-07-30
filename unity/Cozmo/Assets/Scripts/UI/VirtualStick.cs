using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using UnityEngine.EventSystems;

/// <summary>
/// this component is wildly more complicated than currently necessary, as it was originally used for
/// 	an assortment of robot control schemes
/// 
/// 	at this stage, now that the driving controls are established, the virtual sticks could probably be replaced with DynamicSliderFrames
/// </summary>
[ExecuteInEditMode]
public class VirtualStick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler {

#region INSPECTOR FIELDS

	//recenter the stick from touchdown position
	[SerializeField] bool dynamic = true;
	[SerializeField] bool hideEvenIfNotDynamic = false;
	[SerializeField] bool staticOnSmallScreens = true;
	[SerializeField] bool snapWidthToSideBar = true;

	[SerializeField] float sideBarSnapScaler = 1f;
	[SerializeField] RectTransform bg = null;
	[SerializeField] RectTransform stick = null;
	[SerializeField] RectTransform hint = null;
	[SerializeField] RectTransform deadZoneRect = null;
	[SerializeField] RectTransform deadZoneVModeRect = null;
	[SerializeField] RectTransform deadZoneHModeRect = null;
	[SerializeField] RectTransform capTop = null;
	[SerializeField] RectTransform capBottom = null;
	[SerializeField] RectTransform capLeft = null;
	[SerializeField] RectTransform capRight = null;
	[SerializeField] RectTransform originMarker = null;
	[SerializeField] RectTransform bgDefaultMode = null;
	[SerializeField] RectTransform bgVertMode = null;
	[SerializeField] RectTransform bgUpMode = null;
	[SerializeField] RectTransform bgDownMode = null;
	[SerializeField] RectTransform bgSideMode = null;
	[SerializeField] Image stickImageThrown = null;
	[SerializeField] Image stickImageNeutral = null;
	[SerializeField] bool clampRadially = true;
	[SerializeField] Vector2 deadZone = new Vector2(0.01f, 0.01f);
	[SerializeField] bool deadZoneRadial = false;
	[SerializeField] bool scaleMagFromDeadZone = true;
	[SerializeField] float horClampThrow = 1f;
	[SerializeField] float verticalAxisSnapAngle = 0f;
	[SerializeField] float horizontalAxisSnapAngle = 0f;
	[SerializeField] float verticalModeEntryAngle = 0f;
	[SerializeField] bool verticalModeBothWays = false;
	[SerializeField] float horizontalModeEntryAngle = 0f;
	[SerializeField] float verticalModeMaxAngleAllowance = 170f;
	[SerializeField] bool clearHorizontal = true;
	[SerializeField] bool clearVertical = true;
	[SerializeField] float bgWidthInches = -1f;
	[SerializeField] float bgHeightInches = -1f;
	[SerializeField] float stickWidthInches = -1f;
	[SerializeField] float stickHeightInches = -1f;
	[SerializeField] bool allowAxisSwipes = false;
	[SerializeField] bool allowPreciseSwipes = false;
	[SerializeField] float swipeMinSpeed = 4f;
	[SerializeField] bool horizontalOnly = false;
	[SerializeField] bool verticalOnly = false;
	[SerializeField] bool scaleToScreen = false;
	[SerializeField] float[] swipeVerticalMagnitudes = { 1f };
	[SerializeField] float[] swipeHorizontalMagnitudes = { 0.25f, 0.5f, 1f };
//	[SerializeField] TextAnchor smallScreenAnchorType = TextAnchor.MiddleCenter;
	[SerializeField] Vector2 smallScreenAnchorPos = Vector2.zero;
	[SerializeField] float doubleTapDelayMax = 0f;
	[SerializeField] bool forceUpOnly = false;
	[SerializeField] bool forceDownOnly = false;
#endregion

#region PROPERTIES
	public float MaxAngle { get { return verticalModeMaxAngleAllowance; } }
	public Vector2 SwipedDirection { get; private set; }
	public bool SwipeRequest { get; private set; }
	public bool SwipeActive { get { return SwipedDirection.sqrMagnitude > 0f; } }
	public int SwipeIndex { get; private set; }
	public bool UpModeEngaged { get; private set; }
	public bool DownModeEngaged { get; private set; }
	public bool SideModeEngaged { get; private set; }
	public bool VertModeEngaged { get; private set; }
	public bool IsPressed { get; private set; }
	public float PressedTime { get; private set; }
	public bool DoubleTapped { get; private set; }
	public float Horizontal {
		get { 
			
			float hor = JoystickData.x;
			if(Mathf.Abs(hor) > horClampThrow) hor = horClampThrow * Mathf.Sign(hor);
			
			return hor; 
		}
	}
	public float Vertical {
		get { 
			return JoystickData.y; 
		}
	}
	Vector2 ZoneCenter {
		get {

			Vector2 center = Vector2.zero;

			Vector3[] corners = new Vector3[4];
			rTrans.GetLocalCorners(corners);
			if(corners == null || corners.Length == 0) return center;
			return (corners[0] + corners[2]) * 0.5f;
		}
	}
	Vector2 JoystickData {
		get {
			Vector2 res = stick.anchoredPosition;
			
			if(clampRadially && !horizontalOnly && !verticalOnly && !SideModeEngaged && !VertModeEngaged) {
				res = res / radius;
			}
			else {
				if(bg.rect.width > 0f) res.x /= bg.rect.width * 0.5f;
				if(bg.rect.height > 0f) res.y /= bg.rect.height * 0.5f;
			}
			
			//Debug.Log("res.x("+res.x+") deadZone.x("+deadZone.x+")");
			
			if(deadZoneRadial && !horizontalOnly && !verticalOnly && !SideModeEngaged && !VertModeEngaged) {
				if(res.magnitude < deadZone.x) {
					res = Vector2.zero;
				}
				else if(scaleMagFromDeadZone && deadZone.x > 0f) {
					//smooth scale magnitude from deadzone to full throw
					res = res.normalized * Mathf.Lerp(0f, 1f, (res.magnitude - deadZone.x) / (1f - deadZone.x));
				}
			}
			else {
				if(Mathf.Abs(res.x) < deadZone.x) {
					res.x = 0f;
				}
				else if(scaleMagFromDeadZone && deadZone.x > 0f) {
					//smooth scale magnitude from deadzone to full throw
					res.x = Mathf.Sign(res.x) * Mathf.Lerp(0f, 1f, (Mathf.Abs(res.x) - deadZone.x) / (1f - deadZone.x));
				}
				
				if(Mathf.Abs(res.y) < deadZone.y) {
					res.y = 0f;
				}
				else if(scaleMagFromDeadZone && deadZone.y > 0f) {
					//smooth scale magnitude from deadzone to full throw
					res.y = Mathf.Sign(res.y) * Mathf.Lerp(0f, 1f, (Mathf.Abs(res.y) - deadZone.y) / (1f - deadZone.y));
				}
			}
			
			return res;
		}
	}
#endregion

#region MISC MEMBERS
	float radius;
	RectTransform rTrans;
	ScreenOrientation orientation;
	float lastScreenWidth = 0f;
	float lastScreenHeight = 0f;
	Vector2 touchDownValue = Vector2.zero;
	bool wasClockwise = false;
	bool wasCounterClockwise = false;
	Vector2 oldSwipe;
	float pointerDownTime = 0f;
	Canvas canvas;	
	CanvasScaler canvasScalar;	
	Camera canvasCamera;
	float screenScaleFactor = 1f;
#endregion

#region COMPONENT CALLBACKS
	void OnEnable() {
		canvas = GetComponentInParent<Canvas>();
		canvasScalar = canvas.gameObject.GetComponent<CanvasScaler>();
		rTrans = transform as RectTransform;
		canvasCamera = canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null;

		ResizeToScreen();
		if(!Application.isPlaying) return;
		//Debug.Log ("Joystick Awake startPosition(" + startPosition + ") stick.position("+stick.position+") stick.anchoredPosition("+stick.anchoredPosition+")");
		bg.gameObject.SetActive(!dynamic && !hideEvenIfNotDynamic);
		if(hint != null) hint.gameObject.SetActive(!bg.gameObject.activeSelf);
		stick.gameObject.SetActive(!dynamic && !hideEvenIfNotDynamic);

		SwipeIndex = 0;
	}

	void Update() {
		if(	Screen.orientation != orientation || lastScreenWidth != Screen.width || lastScreenHeight != Screen.height ) {
			ResizeToScreen();
		}
		
		if(!Application.isPlaying) return;
		
		if(IsPressed) {
			PressedTime += Time.deltaTime;
		}
		else {
			PressedTime = 0f;
			
			//if we've allowed a swipe to override stick, then snap it here
			if(SwipeActive) {

				float[] mags = swipeVerticalMagnitudes;
				if(Vector2.Dot(SwipedDirection, Vector2.right) != 0f) {
					mags = swipeHorizontalMagnitudes;
				}

				float mag = 1f;
				SwipeIndex = Mathf.Clamp(SwipeIndex, 0, mags.Length-1);
				mag = mags[SwipeIndex];
				
				Vector2 pos = SwipedDirection * radius * mag;
				stick.anchoredPosition = pos;
			}
			else {
				SwipeIndex = 0;
			}
		}
		
		RefreshStickImages();
		RefreshBGImages();
		RefreshCaps();
		
	}
#endregion

#region PRIVATE METHODS
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

	void ResizeToScreen() {

		float dpi = Screen.dpi;

		//Debug.Log("ResizeToScreen dpi("+dpi+")"); //326
		if(dpi == 0f) return;

		float refW = Screen.width;
		float refH = Screen.height;

		screenScaleFactor = 1f;

		if(canvasScalar != null) {
			screenScaleFactor = canvasScalar.referenceResolution.y / Screen.height;
			refW = canvasScalar.referenceResolution.x;
			refH = canvasScalar.referenceResolution.y;
		}

		float refAspect = refW / refH;
		float actualAspect = (float)Screen.width / (float)Screen.height;
		
		float totalRefWidth = (refW / refAspect) * actualAspect;
		float sideBarWidth = (totalRefWidth - refW) * 0.5f;

		float screenHeightInches = (float)Screen.height / (float)dpi;

		if(scaleToScreen) {
			rTrans.anchoredPosition = Vector2.zero;
			float length = Mathf.Min(refW, refH);
			rTrans.sizeDelta = new Vector2(length, length);
		}

		if(bgWidthInches > 0f && bgHeightInches > 0f) {
			Vector3 size = bg.sizeDelta;
			size.x = Mathf.Clamp(screenScaleFactor * dpi * bgWidthInches, 0f, rTrans.rect.width);// * 0.75f);
			size.y = Mathf.Clamp(screenScaleFactor * dpi * bgHeightInches, 0f, rTrans.rect.height);// * 0.75f);
			bg.sizeDelta = size;
		}

		if(stickWidthInches > 0f && stickHeightInches > 0f) {
			Vector3 size = stick.sizeDelta;
			size.x = Mathf.Clamp(screenScaleFactor * dpi * stickWidthInches, 0f, refW);
			size.y = Mathf.Clamp(screenScaleFactor * dpi * stickHeightInches, 0f, refH);
			stick.sizeDelta = size;

			if(capTop != null) capTop.sizeDelta = size;
			if(capBottom != null) capBottom.sizeDelta = size;
			if(capLeft != null) capLeft.sizeDelta = size;
			if(capRight != null) capRight.sizeDelta = size;

			if(originMarker != null) originMarker.sizeDelta = size;

			if(deadZoneRect != null) {
				deadZoneRect.sizeDelta = size;
			}

			if(deadZoneHModeRect != null) {
				deadZoneHModeRect.sizeDelta = size;
			}

			if(deadZoneVModeRect != null) {
				deadZoneVModeRect.sizeDelta = size;
			}
		}

		bg.anchoredPosition = Vector3.zero;

		float height = bg.rect.height;
		float width = bg.rect.width;

		if(capTop != null) capTop.anchoredPosition = Vector2.up * height * 0.5f;
		if(capBottom != null) capBottom.anchoredPosition = -Vector2.up * height * 0.5f;
		if(capLeft != null) capLeft.anchoredPosition = -Vector2.right * width * 0.5f;
		if(capRight != null) capRight.anchoredPosition = Vector2.right * width * 0.5f;

		if(originMarker != null) originMarker.anchoredPosition = Vector2.zero;

		if(deadZoneRect != null) {
			Vector3 size = deadZoneRect.sizeDelta;
			if(deadZoneRadial) {
				size.x = size.y = deadZone.x * width;
			}
			else {
				if(deadZone.x > 0f && !verticalOnly) size.x = deadZone.x * width;
				if(deadZone.y > 0f && !horizontalOnly) size.y = deadZone.y * height;
			}

			deadZoneRect.sizeDelta = size;
		}

		if(deadZoneHModeRect != null) {
			Vector3 size = deadZoneHModeRect.sizeDelta;
			size.x = deadZone.x * width;
			deadZoneHModeRect.sizeDelta = size;
		}

		if(deadZoneVModeRect != null) {
			Vector3 size = deadZoneVModeRect.sizeDelta;
			size.y = deadZone.y * height;
			deadZoneVModeRect.sizeDelta = size;
		}

		RefreshRadius();

		lastScreenWidth = Screen.width;
		lastScreenHeight = Screen.height;
		orientation = Screen.orientation;

		if(staticOnSmallScreens && Application.isPlaying && screenHeightInches < CozmoUtil.SMALL_SCREEN_MAX_HEIGHT) {
			dynamic = false;

			if(snapWidthToSideBar) {
				Vector2 size = rTrans.sizeDelta;
				size.x = sideBarWidth * sideBarSnapScaler;
				rTrans.sizeDelta = size;
				smallScreenAnchorPos.x = sideBarWidth * 0.5f - size.x * 0.5f;
			}

			bg.anchoredPosition = smallScreenAnchorPos;
			//Debug.Log("pivot("+bg.pivot+") smallScreenAnchorPos("+smallScreenAnchorPos+")");
		}

		//Debug.Log("screenHeightInches("+screenHeightInches+") dynamic("+dynamic+")");

		stick.anchoredPosition = Vector2.zero;
	}

	void RefreshStickImages() {
		if(stickImageThrown == null) return;
		if(stickImageNeutral == null) return;

		Vector2 throwVector = JoystickData;

		bool thrown = throwVector.sqrMagnitude > 0f;
		if(horizontalOnly || SideModeEngaged) thrown = throwVector.x != 0f;
		if(verticalOnly || VertModeEngaged) thrown = throwVector.y != 0f;

		stickImageThrown.enabled = thrown;
		stickImageNeutral.enabled = !thrown;

		//Debug.Log("RefreshStickImages thrown("+thrown+") throwVector("+throwVector.x +", "+throwVector.y+") SideModeEngaged("+SideModeEngaged+")");

		if(thrown) {
			float angle = Vector2.Angle(Vector3.up, throwVector);
			if(Vector2.Dot(Vector2.right, throwVector) > 0f) {
				angle = -angle;
			}

			stickImageThrown.rectTransform.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);
		}
	}

	void RefreshBGImages() {
		if(bgDefaultMode != null) bgDefaultMode.gameObject.SetActive(!UpModeEngaged && !DownModeEngaged && !SideModeEngaged && !VertModeEngaged);

		if(bgVertMode != null) bgVertMode.gameObject.SetActive(VertModeEngaged);
		if(bgUpMode != null) bgUpMode.gameObject.SetActive(UpModeEngaged);
		if(bgDownMode != null) bgDownMode.gameObject.SetActive(DownModeEngaged);
		if(bgSideMode != null) bgSideMode.gameObject.SetActive(SideModeEngaged);


		if(deadZoneHModeRect != null) deadZoneHModeRect.gameObject.SetActive(SideModeEngaged);
		if(deadZoneVModeRect != null) deadZoneVModeRect.gameObject.SetActive(VertModeEngaged);
	}

	void RefreshCaps() {

		if(!UpModeEngaged && !DownModeEngaged && !SideModeEngaged && !VertModeEngaged) {
			if(capTop != null) capTop.gameObject.SetActive(true);
			if(capBottom != null) capBottom.gameObject.SetActive(true);
			if(capLeft != null) capLeft.gameObject.SetActive(true);
			if(capRight != null) capRight.gameObject.SetActive(true);
		}
		else {
			if(capTop != null) capTop.gameObject.SetActive(UpModeEngaged || VertModeEngaged);
			if(capBottom != null) capBottom.gameObject.SetActive(DownModeEngaged || VertModeEngaged);
			if(capLeft != null) capLeft.gameObject.SetActive(SideModeEngaged);
			if(capRight != null) capRight.gameObject.SetActive(SideModeEngaged);
		}
	}

	void ConsiderSwipe(bool underway) {
		SwipedDirection = Vector2.zero;
		if(!allowAxisSwipes && !allowPreciseSwipes) return;
		
		if(PressedTime <= 0f) {
			return;
		}
		if(PressedTime > 1f) {
			return;
		}
		
		Vector2 delta = JoystickData - touchDownValue;
		float distance = delta.magnitude;
		
		if(distance < swipeMinSpeed * PressedTime) return;

		SwipedDirection = delta.normalized;
		
		//if our swipe dir has changed, start our stepwise magnitude index over
		if(!underway && oldSwipe.sqrMagnitude > 0f) {
			SwipeIndex++;
			
			if(Vector2.Angle(oldSwipe, SwipedDirection) > 10f) {
				SwipeIndex = 0;
			}
		}
		
		if(allowPreciseSwipes && !horizontalOnly && !verticalOnly) return;
		
		float dotUp = Vector2.Dot(SwipedDirection, Vector2.up);
		
		if(verticalOnly || VertModeEngaged) {
			if(dotUp >= 0f) {
				SwipedDirection = Vector2.up;
			}
			else {
				SwipedDirection = -Vector2.up;
			}
			return;
		}
		
		float dotRight = Vector2.Dot(SwipedDirection, Vector2.right);
		
		if(horizontalOnly || SideModeEngaged) {
			if(dotRight >= 0f) {
				SwipedDirection = Vector2.right;
			}
			else {
				SwipedDirection = -Vector2.right;
			}
			return;
		}
		
		if(Mathf.Abs(dotUp) >= Mathf.Abs(dotRight)) {
			if(dotUp >= 0f) {
				SwipedDirection = Vector2.up;
			}
			else { 
				SwipedDirection = -Vector2.up;
			}
		}
		else if(dotRight >= 0f) {
			SwipedDirection = Vector2.right;
		}
		else {
			SwipedDirection = -Vector2.right;
		}
	}

	void ProcessStick(Vector2 pointerPos) {
		Vector2 delta = pointerPos - bg.anchoredPosition - ZoneCenter; //StickCenterOnScreen;

		if(clampRadially) {
			delta = Vector3.ClampMagnitude(delta, radius);
		}
		else {
			delta.x = Mathf.Clamp(Mathf.Abs(delta.x), 0f, bg.rect.width * 0.5f) * Mathf.Sign(delta.x);
			delta.y = Mathf.Clamp(Mathf.Abs(delta.y), 0f, bg.rect.height * 0.5f) * Mathf.Sign(delta.y);
		}

		if(delta.sqrMagnitude <= 0f) {
			stick.anchoredPosition = Vector2.zero;
			return;
		}

		if(horizontalOnly || SideModeEngaged) {
			delta.y = 0f;
		}
		
		if(verticalOnly || VertModeEngaged) {
			delta.x = 0f;
		}

		if(UpModeEngaged) {

			if(verticalModeMaxAngleAllowance > 0f) {
				float angle = Vector2.Angle(Vector2.up, delta.normalized);

				if(angle > 90f && !wasClockwise && !wasCounterClockwise) {
					wasClockwise = delta.x > 0f;
					wasCounterClockwise = delta.x < 0f;
	 			}
				else if(angle <= 90f) { //release the rot direction locks if we have returned to the top half of circle
					wasClockwise = false;
					wasCounterClockwise = false;
				}

				if(angle > verticalModeMaxAngleAllowance || (wasClockwise && delta.x <= 0f) || (wasCounterClockwise && delta.x >= 0f)) {
					float signedAngle = verticalModeMaxAngleAllowance * (wasClockwise ? -1f : 1f);
					delta = Quaternion.AngleAxis(signedAngle, Vector3.forward) * Vector2.up * delta.magnitude;
				}
			}
			else {
				delta.y = Mathf.Max(0f, delta.y);
			}
		}
		else if(DownModeEngaged) {

			if(verticalModeMaxAngleAllowance > 0f) {
				float angle = Vector2.Angle(-Vector2.up, delta.normalized);

				if(angle > 90f && !wasClockwise && !wasCounterClockwise) {
					wasClockwise = delta.x < 0f;
					wasCounterClockwise = delta.x > 0f;
				}
				else if(angle <= 90f) { //release the rot direction locks if we have returned to the bottom half of circle
					wasClockwise = false;
					wasCounterClockwise = false;
				}

				if(angle > verticalModeMaxAngleAllowance || (wasClockwise && delta.x >= 0f) || (wasCounterClockwise && delta.x <= 0f)) {
					float signedAngle = verticalModeMaxAngleAllowance * (wasClockwise ? -1f : 1f);
					delta = Quaternion.AngleAxis(signedAngle, Vector3.forward) * -Vector2.up * delta.magnitude;
				}
			}
			else {
				delta.y = Mathf.Min(0f, delta.y);
			}
		}

		//only vert snap if not in mode or pointed in mode direction
		if(verticalAxisSnapAngle > 0f && ( (!UpModeEngaged || delta.y > 0f) && (!DownModeEngaged || delta.y < 0f))) {
			float angleFromUp = Vector2.Angle(Vector2.up, delta.normalized);
			
			if(angleFromUp <= verticalAxisSnapAngle) {
				delta = Vector2.up * delta.magnitude;
			}
			else if(angleFromUp >= 180f - verticalAxisSnapAngle) {
				delta = -Vector2.up * delta.magnitude;
			}
		}
		
		if(horizontalAxisSnapAngle > 0f) {
			float angleFromRight = Vector2.Angle(Vector2.right, delta.normalized);
			
			if(angleFromRight <= horizontalAxisSnapAngle) {
				delta = Vector2.right * delta.magnitude;
			}
			else if(angleFromRight >= 180f - horizontalAxisSnapAngle) {
				delta = -Vector2.right * delta.magnitude;
			}
		}

		stick.anchoredPosition = delta;
		//Debug.Log ("Joystick pointerPos(" + pointerPos + ") radius(" + radius + ") delta(" + delta + ") bg.anchoredPosition("+bg.anchoredPosition+") stick.anchoredPosition("+stick.anchoredPosition+")");
	}
	
	void CheckForModeEngage() {
		if(forceUpOnly) {
			UpModeEngaged = true;
			return;
		}
		if(forceDownOnly) {
			DownModeEngaged = true;
			return;
		}

		if(verticalModeEntryAngle <= 0f && horizontalModeEntryAngle <= 0f)
			return;
		
		if(UpModeEngaged) return;
		if(DownModeEngaged) return;
		if(SideModeEngaged) return;
		if(VertModeEngaged) return;

		Vector2 dragThusFar = JoystickData - touchDownValue;
		if(dragThusFar.sqrMagnitude <= 0f) return;
		
		dragThusFar.Normalize();
		float angleFromUp = Vector2.Angle(Vector2.up, dragThusFar);
		
		if(angleFromUp <= verticalModeEntryAngle) {
			if(verticalModeBothWays) {
				VertModeEngaged = true;
			}
			else {
				UpModeEngaged = true;
			}
			return;
		}
		
		if(angleFromUp >= 180f - verticalModeEntryAngle) {
			if(verticalModeBothWays) {
				VertModeEngaged = true;
			}
			else {
				DownModeEngaged = true;
			}
			return;
		}
		
		if(angleFromUp >= 90f - horizontalModeEntryAngle && angleFromUp <= 90f + horizontalModeEntryAngle) {
			SideModeEngaged = true;
			return;
		}
	}
#endregion

#region PUBLIC METHODS
	public void EndSwipe() {

		SwipedDirection = Vector2.zero;
		SwipeRequest = false;
		SwipeIndex = 0;
		stick.anchoredPosition = Vector2.zero;
	}

	public void AbsorbSwipeRequest() {
		Debug.Log("AbsorbSwipeRequest()");
		SwipeRequest = false;
	}

	public void AbsorbDoubleTap() {
		Debug.Log("AbsorbDoubleTap()");
		DoubleTapped = false;
	}

	public void OnPointerDown(PointerEventData eventData) {

		Vector2 localTouchPos;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, eventData.position, canvasCamera, out localTouchPos);
		//Debug.Log("OnPointerDown eventData.position("+eventData.position+") localTouchPos("+localTouchPos+") canvasCamera("+canvasCamera+")");
		//Vector2 pointerPos = eventData.position * screenScaleFactor;

		float newTime = Time.time;
		if(doubleTapDelayMax > 0f) {
			if(pointerDownTime != 0f && (newTime - pointerDownTime) <= doubleTapDelayMax) {
				DoubleTapped = true;
			}
		}

		pointerDownTime = newTime;

		if(dynamic) {
			bg.anchoredPosition = localTouchPos - ZoneCenter;//pointerPos - ZoneCenter;
			//Debug.Log("bg.anchoredPosition("+bg.anchoredPosition+") position("+pointerPos+") ZoneCenter("+ZoneCenter+")");
			stick.anchoredPosition = Vector2.zero;
		}
		
		bg.gameObject.SetActive(true);
		stick.gameObject.SetActive(true);
		if(hint != null) hint.gameObject.SetActive(!bg.gameObject.activeSelf);

		oldSwipe = SwipedDirection;
		if(SwipedDirection.sqrMagnitude == 0f) SwipeIndex = 0;
		SwipeRequest = false;
		IsPressed = true;
		PressedTime = 0f;
		
		ProcessStick(localTouchPos);
		
		touchDownValue = JoystickData;
		
		UpModeEngaged = false;
		DownModeEngaged = false;
		SideModeEngaged = false;
		VertModeEngaged = false;

		wasClockwise = false;
		wasCounterClockwise = false;

		//Debug.Log ("Joystick OnPointerDown pointerPos(" + pointerPos + ") ZoneCenter("+ZoneCenter+") disallowSwipeForThisTouch("+disallowSwipeForThisTouch+")");
	}
	
	public void OnDrag(PointerEventData eventData) {
		Vector2 localTouchPos;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, eventData.position, canvasCamera, out localTouchPos);

		ProcessStick(localTouchPos);//eventData.position * screenScaleFactor);
		CheckForModeEngage();
		ConsiderSwipe(true);
	}

	public void OnPointerUp(PointerEventData eventData) {
		Vector2 localTouchPos;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, eventData.position, canvasCamera, out localTouchPos);


		ProcessStick(localTouchPos); //eventData.position * screenScaleFactor);
		ConsiderSwipe(false);

		Vector2 throwVector = stick.anchoredPosition;
		
		//if(dynamic) bg.anchoredPosition = Vector3.zero;
		
		if(clearHorizontal) {
			throwVector.x = 0f;
		}
		
		if(clearVertical) {
			throwVector.y = 0f;
		}

		stick.anchoredPosition = throwVector;
		
		IsPressed = false;
		PressedTime = 0f;
		SwipeRequest = SwipedDirection.sqrMagnitude > 0f;
		bg.gameObject.SetActive((!dynamic && !hideEvenIfNotDynamic) || SwipeRequest);
		if(hint != null) hint.gameObject.SetActive(!bg.gameObject.activeSelf);

		stick.gameObject.SetActive((!dynamic && !hideEvenIfNotDynamic) || SwipeRequest);
		//Debug.Log ("Joystick OnPointerUp StickCenterOnScreen(" + StickCenterOnScreen + ") stick.anchoredPosition("+stick.anchoredPosition+")");
		
		UpModeEngaged = false;
		DownModeEngaged = false;
		SideModeEngaged = false;
		VertModeEngaged = false;
	}

	public void SetForceReverse(bool reverse) {
		forceDownOnly = reverse;
		forceUpOnly = !reverse;
	}

#endregion

}
