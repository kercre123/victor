using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System;
using System.Collections;
using System.Collections.Generic;


public class SpinWheel : MonoBehaviour, IPointerDownHandler, IPointerUpHandler {

	#region NESTED DEFINITIONS

	struct SpinSample {
		public float deltaTime;
		public float deltaAngle;
	}

	#endregion

	#region INSPECTOR FIELDS

	[SerializeField] RectTransform wheel = null;
	[SerializeField] RectTransform peg = null;
	[SerializeField] RectTransform pegBase = null;
	
	[SerializeField] int numSlices = 8;
	[SerializeField] Transform slicePrefab;
	[SerializeField] Transform pegPrefab;
	[SerializeField] AudioClip pegSound;
	[SerializeField] AudioClip spinLoopSound;

	[SerializeField] InputField inputField_numSlices = null;

	[SerializeField] float maxAngularVelocity = 3600f;
	[SerializeField] float fadeBelowAngularVelocity = 2f;
	[SerializeField] float minAngularVelocity = 1f;
	[SerializeField] float minStartingVelocity = 360f;
	[SerializeField] float spinMultiplier = 2f;
	[SerializeField] float dragCoefficient = 0.3f;
	[SerializeField] int maxSamples = 5;
	[SerializeField] float pegSlowDownThreshold = 360f;
	[SerializeField] float pegSlowDownFactor = 0.95f;
	[SerializeField] float pegTraversalCostFactor = 4f;
	[SerializeField] float pegBounceBelowVel = 5f;
	[SerializeField] float pegBounceVelFactor = 0.25f;
	[SerializeField] float pegDegrees = 1f;
	[SerializeField] float pegBendAngle = 30f;

	[SerializeField] Color[] imageColors = { Color.white, Color.black };
	[SerializeField] Color[] spokeColors = { Color.black, Color.white };
	[SerializeField] Color[] textColors = { Color.black , Color.white };
	[SerializeField] Color[] outlineColors = { Color.black , Color.white };

	[SerializeField] float tokenRadius = 300f;
	[SerializeField] float tokenOuterRadius = 300f;

	#endregion

	#region PUBLIC MEMBERS

	public int GetCurrentNumber() { return currentSlice != null ? currentSlice.Number : 0; }

	public bool Finished { get; private set; }
	public bool SpinUnderway { get { return angularVelocity != 0f && !dragging; } }

	public float Speed { get { return Mathf.Abs(angularVelocity / 360f); } }
	public float TokenRadius { get { return tokenRadius; } }
	public float TokenOuterRadius { get { return tokenOuterRadius; } }

	#endregion

	#region PRIVATE MEMBERS

	float angularVelocity = 0f;
	float pegBendFactor = 0f;

	List<SpinSample> samples = new List<SpinSample>();
	List<PieSlice> slices = new List<PieSlice>();
	List<RectTransform> pegs = new List<RectTransform>();

	AudioSource loopingAudioSource = null;
	RectTransform rTrans = null;
	Canvas canvas = null;

	Vector2 lastTouchPos = Vector2.zero;
	float lastTime = 0f;
	float lastAngle = 0f;
	int lastNumber = 0;

	int currentSliceIndex = 0;

	bool pegHit = false;
	bool pegTouching = false;
	bool dragging = false;
	bool locked = false;
	bool allowTouches = false;

	PieSlice currentSlice { get { return (currentSliceIndex >= 0 && currentSliceIndex < slices.Count) ? slices[currentSliceIndex] : null; } }

	float currentSliceCenterAngle { get { return currentSliceIndex*sliceArc + sliceArc*0.5f; } }


	int GetSliceIndexAtAngle(float angle) {
		
		angle = MathUtil.ClampAngle0_360(-angle);

		int index = Mathf.FloorToInt(angle / sliceArc);
		if(index < 0 || index >= slices.Count) return 0;
		
		return index;
	}

	float sliceArc { get { return 360f / numSlices; } }

	float _currentAngle = 0f;
	float currentAngle {
		get {
			return _currentAngle;
		}		
		set {
			_currentAngle = value;
			pegTouching = PegTouching();
			if(!pegTouching) currentSliceIndex = GetSliceIndexAtAngle(_currentAngle);
		}		
	}
	
	Image _dragImage;
	Image dragImage { 
		get {
			if(_dragImage == null) _dragImage = GetComponent<Image>();
			return _dragImage;
		}
	}

	#endregion
	
	#region MONOBEHAVIOR CALLBACKS

	void Awake() {
		rTrans = transform as RectTransform;
		canvas = GetComponentInParent<Canvas>();

		Application.targetFrameRate = 60;
		Time.fixedDeltaTime =  1f / 60f;
	}

	void OnEnable() {
		//RefreshSettings();
		//OptionsScreen.RefreshSettings += RefreshSettings;
		SetNumSlices(numSlices);
		ResetWheel();
		currentSliceIndex = GetSliceIndexAtAngle(currentAngle);
	}

	void OnDisable() {
		//OptionsScreen.RefreshSettings -= RefreshSettings;
		StopLoopingSound();
	}

	void FixedUpdate() {
		//Debug.Log("SpinWheel FixedUpdate");

		if(locked) return;
		if(dragging) return;

		bool wasSpinning = SpinUnderway;

		DecayVelocity(Time.deltaTime);

		if(SpinUnderway) {
			SpinUpdate(Time.time);
		}
		else if(wasSpinning) {
			SpinEnd();
		}
	}

	void Update() {

		if(dragging) {
			//doing this in update instead of OnDrag callback so as to sample from a reasonable deltaTime
			DragUpdate(Input.mousePosition, Time.time);
		}

		//Debug.Log("SpinWheel FixedUpdate");
		RefreshDisplay(pegHit, SpinUnderway);
		lastAngle = currentAngle;
		pegHit = false;

	}

	#endregion

	#region PRIVATE METHODS

	void SpinStart() {
		Finished = false;
		//do effect?
	}

	void SpinUpdate(float time, bool prediction=false) {
		//currentAngle = 
		ApplySpin(time - lastTime);
		lastTime = time;
	}

	void SpinEnd() {
		StopLoopingSound();
		Finished = true;
	}

	void RefreshDisplay(bool doPegSound, bool doLoopSound) {

		wheel.localRotation = Quaternion.AngleAxis(currentAngle, Vector3.forward);

		if(doPegSound && pegSound != null && Mathf.Abs(angularVelocity) < pegSlowDownThreshold) {
			//Debug.Log ("frame("+Time.frameCount+") pegSound!");
			float pitchFactor = Mathf.Clamp01(Mathf.Abs(angularVelocity) / pegSlowDownThreshold);
			float volumeFactor = 1f - Mathf.Clamp01(Mathf.Abs(angularVelocity) / pegSlowDownThreshold);
			
			float p = Mathf.Lerp(0.75f, 1.5f, pitchFactor);
			float v = Mathf.Lerp(0f, 1f, volumeFactor);
			
			AudioManager.PlayOneShot(pegSound, 0f, p, v);
		}
		
		if(doLoopSound && spinLoopSound != null) {
			if(loopingAudioSource == null) {
				loopingAudioSource = AudioManager.PlayAudioClipLooping(spinLoopSound, 0f, AudioManager.Source.Gameplay);
			}
			float pitchFactor = Mathf.Abs(angularVelocity) / (pegSlowDownThreshold * 2f);
			float volumeFactor = Mathf.Abs(angularVelocity) / pegSlowDownThreshold;
			loopingAudioSource.pitch = Mathf.Lerp(0.5f, 2f, pitchFactor);
			loopingAudioSource.volume = Mathf.Lerp(0f, 1f, volumeFactor);
		}

		RefreshPegAngle();
	}
	
	void StopLoopingSound() {
		if(spinLoopSound == null) return;
		if(loopingAudioSource == null) return;
		AudioManager.Stop(spinLoopSound);
		loopingAudioSource = null;
	}
	
	void DecayVelocity(float deltaTime) {
		if(angularVelocity == 0f) return;

		if(Mathf.Abs(angularVelocity) > maxAngularVelocity) {
			angularVelocity = maxAngularVelocity * Mathf.Sign(angularVelocity);
		}

		float reduction = angularVelocity * dragCoefficient * deltaTime;

		angularVelocity -= reduction;

		if(Mathf.Abs (angularVelocity) <= minAngularVelocity) angularVelocity = 0f;
		if(Mathf.Abs (angularVelocity) <= fadeBelowAngularVelocity) angularVelocity = Mathf.Lerp(Mathf.Abs(angularVelocity), 0f, 0.25f) * Mathf.Sign(angularVelocity);
	}

	//recursively apply spin angle for this frame, with consideration for the effects that pegs have on our
	//angle and velocity
	float pegRepulsion = 10f;
	void ApplySpin(float deltaTime) {
		if(angularVelocity == 0f) return;
		if(deltaTime <= 0f) return;

		float deltaAngle = angularVelocity * deltaTime;

		if(Mathf.Abs(angularVelocity) > pegSlowDownThreshold) {
			pegHit = true;
		}
		else if(PegTouching()) {
			float pegRepulsionFactor = -Mathf.Clamp01(degToEndOfPeg / pegDegrees) * pegRepulsion * deltaTime;
			angularVelocity += pegRepulsionFactor;
			deltaAngle = angularVelocity * deltaTime;
			pegHit = true;
		}
		else {
			int pegsCrossed = PegsCrossed(lastAngle, currentAngle+deltaAngle);

			if(pegsCrossed > 0) {
				pegHit = true;
			}
		}

		currentAngle += deltaAngle;		
//				float modArc = (currentAngle + pegDegrees * 0.5f * Mathf.Sign(angularVelocity)) % sliceArc;
//				float angleToNextPeg = 0f;
//				if(Mathf.Sign(angularVelocity) >= 0) {
//					angleToNextPeg = sliceArc - modArc;
//				}
//				else {
//					angleToNextPeg = modArc;
//				}
//				
//				//step through each peg traversed and remove angular vel as necessary
//				if(angleToNextPeg > 0f && Mathf.Abs(deltaAngle) > angleToNextPeg) {
//					//remove the time we've spent to get to the peg
//					deltaTime -= deltaTime * Mathf.Abs(angleToNextPeg / deltaAngle);
//					float traversedToPeg = angleToNextPeg * Mathf.Sign(angularVelocity);
//					
//					//add the angle we've traversed thus far
//					currentAngle += traversedToPeg;
//					deltaAngle -= traversedToPeg;
//					
//					//if our energy will get us passed this peg, 
//					float pegTraversalCost = pegDegrees * pegTraversalCostFactor;
//					float portionOfPegTraversed = Mathf.Clamp01(Mathf.Abs(deltaAngle) / pegTraversalCost);
//
//					currentAngle += portionOfPegTraversed * pegDegrees * Mathf.Sign(angularVelocity);
//					deltaTime -= deltaTime * Mathf.Abs(portionOfPegTraversed * pegTraversalCost / deltaAngle);
//
//					pegHit = true;
//
//					//slowdown due to peg traversal?
//					angularVelocity = Mathf.Lerp(angularVelocity, angularVelocity * pegSlowDownFactor, Mathf.Abs(portionOfPegTraversed) / pegDegrees);
//
//					if(Mathf.Abs(angularVelocity) < pegBounceBelowVel) {
//						angularVelocity = -angularVelocity * pegBounceVelFactor;
//					}
//					
//					deltaAngle = angularVelocity * deltaTime;
//				}
//			}

//		}


	}

	void RefreshSlices() {

		while(slices.Count < numSlices) {
			Transform slice = (Transform)GameObject.Instantiate(slicePrefab);
			slice.SetParent(wheel, false);
			slices.Add(slice.gameObject.GetComponent<PieSlice>());
		}

		while(slices.Count > numSlices) {
			GameObject.Destroy(slices[0].gameObject);
			slices.RemoveAt(0);
		}

		if(numSlices > 0) {
			float fillAmount = 1f / numSlices;
			for(int i=0;i<numSlices;i++) {
				Color imageColor = imageColors[i % imageColors.Length];
				Color textColor = textColors[i % textColors.Length];
				Color outlineColor = outlineColors[i % outlineColors.Length];

				slices[i].Initialize(fillAmount, sliceArc*i, sliceArc * 0.5f, (i % 4) + 1, imageColor, textColor, outlineColor);
			}
		}

		inputField_numSlices.text = numSlices.ToString();
	}

	void RefreshPegs() {
		
		numSlices = Mathf.Max(0, numSlices);
		
		while(pegs.Count < numSlices) {
			Transform peg = (Transform)GameObject.Instantiate(pegPrefab);
			peg.SetParent(wheel, false);
			pegs.Add(peg.transform as RectTransform);
		}
		
		while(pegs.Count > numSlices) {
			GameObject.Destroy(pegs[0].gameObject);
			pegs.RemoveAt(0);
		}
		
		if(numSlices > 0) {
			for(int i=0;i<numSlices;i++) {
				pegs[i].localRotation = Quaternion.AngleAxis(sliceArc*i, Vector3.forward);
				
				Image pegImage = pegs[i].gameObject.GetComponentInChildren<Image>();
				pegImage.color = spokeColors[i % spokeColors.Length];
			}
		}

	}

	void RefreshSettings() {
		spinMultiplier = PlayerPrefs.GetFloat("SpinMultiplier", 2f);
		minStartingVelocity = PlayerPrefs.GetFloat("SpinnerMinStartingVelocityDegPS", 360f);
		maxAngularVelocity = PlayerPrefs.GetFloat("SpinnerMaxAngularVelocityDegPS", 3600f);
		minAngularVelocity = PlayerPrefs.GetFloat("SpinnerMinAngularVelocityDegPS", 1f);

		dragCoefficient = PlayerPrefs.GetFloat("DragCoefficient", 0.3f);
		maxSamples = PlayerPrefs.GetInt("MaxDragSamples", 5);
		pegSlowDownThreshold = PlayerPrefs.GetFloat("SpinnerPegsSlowDownThreshold", 360f);
	}

	void StartDrag(Vector2 position, float time) {
		//Debug.Log ("SpinWheel StartDrag");
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out lastTouchPos);
		dragging = true;
		angularVelocity = 0f;
		lastTime = time;
		samples.Clear();
	}
	
	void DragUpdate(Vector2 position, float time) {
		//Debug.Log ("SpinWheel DragUpdate position("+position+") time("+time+")");		

		int lastNum = GetCurrentNumber();
		Vector2 touchPos;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out touchPos);
		float deltaAngle = MathUtil.SignedVectorAngle(lastTouchPos, touchPos, Vector3.forward);
		
		currentAngle += deltaAngle;

		float deltaTime = time - lastTime;
		if(deltaTime > 0f) {
			SpinSample sample = new SpinSample();
			sample.deltaTime = deltaTime;
			sample.deltaAngle = deltaAngle;
			samples.Add(sample);
			while(samples.Count > maxSamples) samples.RemoveAt(0);

			angularVelocity = deltaAngle / deltaTime;
			
			int newNum = GetCurrentNumber();
			if(newNum != lastNum) {
				pegHit = true;
			}
			
		}

		lastTouchPos = touchPos;
		lastTime = time;
	}
	
	void EndDrag() {
		//Debug.Log ("SpinWheel EndDrag");

		dragging = false;
		angularVelocity = 0f;
		if(samples.Count < 2) return;
		
		float totalTime = 0f;
		float totalAngle = 0f;
		
		for(int i=1;i<samples.Count;i++) {
			totalTime += samples[i].deltaTime;
			totalAngle += samples[i].deltaAngle;
		}
		
		if(totalTime == 0f) return;
		
		angularVelocity = totalAngle / totalTime;
		angularVelocity *= spinMultiplier;
		
		if(Mathf.Abs(angularVelocity) < minStartingVelocity) {
			angularVelocity = minStartingVelocity * Mathf.Sign(angularVelocity);
		}
		
		//string debugString = "SpinWheel OnPointerUp angularVelocity("+angularVelocity+") samples["+samples.Count+"] { ";
		//for(int i=0;i<samples.Count;i++) {
		//	debugString += (samples[i].deltaAngle / samples[i].deltaTime);
		//	if(i<samples.Count-1) debugString += ", ";
		//}
		//debugString += "}";
		
		//Debug.Log (debugString);
		allowTouches = false;
		SpinStart();
	}
	
	float bendDirection = 0f;
	float degToPeg = 0f;
	float degToEndOfPeg = 0f;
	bool PegTouching() {

		bool touching = false;

		float angle = MathUtil.ClampAngle0_360(-currentAngle);

		bendDirection = Mathf.Sign(MathUtil.AngleDelta(currentSliceCenterAngle, angle));
		//force the wheel peg to an apt angle if it is still within pegDegrees of a slice peg

		int closestPegIndex = Mathf.RoundToInt(angle / sliceArc);
		if(closestPegIndex >= numSlices) closestPegIndex = 0;

		float pegAngle = closestPegIndex*sliceArc;
		float halfPeg = pegDegrees * 0.5f;
		float endOfPegAngle = pegAngle + halfPeg * Mathf.Sign(bendDirection);
		degToPeg = MathUtil.AngleDelta(angle, pegAngle);

		degToEndOfPeg = MathUtil.AngleDelta(angle, endOfPegAngle);
		//arc from current wheel peg angle to the end of slice peg's range, if still pressed against peg, set apt angle
		if(Mathf.Abs(degToPeg) < halfPeg) {
			pegBendFactor = (1f - Mathf.Clamp01(Mathf.Abs(degToEndOfPeg) / pegDegrees)) * bendDirection;	
			touching = true;
		}
		
		for(int i=0;i<numSlices;i++) {
			Color first = (closestPegIndex != i || !touching) ? Color.green : (bendDirection > 0 ? Color.red : Color.magenta);
			Color second = (closestPegIndex != i || !touching) ? Color.green : (bendDirection < 0 ? Color.red : Color.magenta);
			
			Debug.DrawRay(transform.position, Quaternion.AngleAxis(i*sliceArc + pegDegrees * 0.5f, Vector3.forward)*wheel.transform.up*1000f, first);
			Debug.DrawRay(transform.position, Quaternion.AngleAxis(i*sliceArc - pegDegrees * 0.5f, Vector3.forward)*wheel.transform.up*1000f, second);
		}
		

//		Debug.Log (	"frame("+Time.frameCount+") "+
//		           	"PegTouching("+touching+") "+
//					"bend("+pegBendFactor+") "+
//		           	"dir("+bendDirection+") "+
//		           	"peg("+closestPegIndex+") "+
//		           	"angle("+angle+") "+
//		            "sliceAngle("+currentSliceCenterAngle+") "+
//		           	"sliceIndex("+currentSliceIndex+") "+
//		            "endOfPegAngle("+endOfPegAngle+")");

		
		return touching;
	}

	int PegsCrossed(float start, float end) {
		//invert angles to match slices and pegs
		start = -start;
		end = -end;

		float dir = Mathf.Sign(MathUtil.AngleDelta(start, end));

		int crossed = 0;

		for(int i=0;i<numSlices;i++) {
			float pegAngle = i*sliceArc;
		}
//
//		int closestPegIndex = Mathf.RoundToInt(angle / sliceArc);
//		if(closestPegIndex >= numSlices) closestPegIndex = 0;
//		
//		float pegAngle = closestPegIndex*sliceArc;
//		float halfPeg = pegDegrees * 0.5f;
//		float endOfPegAngle = pegAngle + halfPeg * Mathf.Sign(bendDirection);
//		degToPeg = MathUtil.AngleDelta(angle, pegAngle);
//		
//		degToEndOfPeg = MathUtil.AngleDelta(angle, endOfPegAngle);
//		//arc from current wheel peg angle to the end of slice peg's range, if still pressed against peg, set apt angle
//		if(Mathf.Abs(degToPeg) < halfPeg) {
//			pegBendFactor = (1f - Mathf.Clamp01(Mathf.Abs(degToEndOfPeg) / pegDegrees)) * bendDirection;	
//			crossed = true;
//		}

		return crossed;
	}

	void RefreshPegAngle() {
		//if we've passed a peg, then just throw it full tilt
		if(!pegTouching && pegHit) {
			pegBendFactor = -Mathf.Sign(angularVelocity);
		}

		if(peg != null) peg.localRotation = Quaternion.AngleAxis(pegBendAngle * pegBendFactor, Vector3.forward);
		
		//decay by default to immitate elasticity
		pegBendFactor = Mathf.Lerp(pegBendFactor, 0f, 0.25f);
		//if(Mathf.Abs(pegBendFactor) < 0.1f) pegBendFactor = 0f;
	}
	

	#endregion

	#region PUBLIC METHODS

	public void Lock() {
		//Debug.Log("Lock wheel("+gameObject.name+")");
		locked = true;
		angularVelocity = 0f;
		dragImage.enabled = false;
		StopLoopingSound();
	}

	public void HidePeg() {
		pegBase.gameObject.SetActive(false);
	}

	public void ShowPeg() {
		pegBase.gameObject.SetActive(true);
	}

	public void Unlock() {
		//Debug.Log("Unlock wheel("+gameObject.name+")");
		locked = false;
		allowTouches = true;
		Finished = false;
		dragImage.enabled = true;
	}

	public void ResetWheel() {
		//Debug.Log("ResetWheel("+gameObject.name+")");

		currentAngle = -sliceArc * 0.5f;

		wheel.localRotation = Quaternion.AngleAxis(currentAngle, Vector3.forward);

		angularVelocity = 0f;
		Finished = false;
		allowTouches = true;
		pegBendFactor = 0f;
	}

	public void SetNumSlices(int num) {
		numSlices = Mathf.Max(0, num);
		RefreshSlices();
		RefreshPegs();
	}

	public void OnSubmitNumSlices() {
		//Debug.Log("OnSubmitNumSlices");
		int newSlices;
		
		if(int.TryParse(inputField_numSlices.text, out newSlices)) {
			if(newSlices != numSlices) {
				//Debug.Log("SetNumSlices");
				SetNumSlices(newSlices);
			}
		}
	}

	#endregion

	#region INTERFACE METHODS	

	public void OnPointerDown(PointerEventData eventData) {
		if(locked) return;
		if(!allowTouches) return;

		StartDrag(eventData.position, Time.time);
	}
	
	public void OnPointerUp(PointerEventData eventData) {
		if(locked) return;
		if(!allowTouches) return;
		
		EndDrag();
	}

	#endregion

}
