using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System;
using System.Collections;
using System.Collections.Generic;


public class SpinWheel : MonoBehaviour, IPointerDownHandler, IPointerUpHandler {

	struct SpinSample {
		public float deltaTime;
		public float deltaAngle;
	}

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
	[SerializeField] bool usePegs = true;
	[SerializeField] float pegSlowDownThreshold = 360f;
	[SerializeField] float pegTraversalCostFactor = 4f;
	[SerializeField] float pegBounceBelowVel = 5f;
	[SerializeField] float pegBounceVelFactor = 0.25f;
	[SerializeField] float pegDegrees = 1f;
	[SerializeField] float pegBendAngle = 30f;

	[SerializeField] Color[] imageColors = { Color.white, Color.black };
	[SerializeField] Color[] textColors = { Color.black , Color.white };

	[SerializeField] float tokenRadius = 300f;

	RectTransform rTrans = null;
	Canvas canvas = null;
	Vector2 lastTouchPos = Vector2.zero;
	float lastTime = 0f;
	float angularVelocity = 0f;
	bool dragging = false;
	bool locked = false;

	List<PieSlice> slices = new List<PieSlice>();
	List<RectTransform> pegs = new List<RectTransform>();

	public bool Finished { get; private set; }
	public bool Spinning { get { return angularVelocity != 0f; } }

	public int GetCurrentNumber() { 
		if(numSlices <= 0) return 0;
		if(slices.Count == 0) return 0;
		
		float z = MathUtil.ClampAngle(wheel.transform.eulerAngles.z);
		if(z < 0) z += 360f;

		int index = Mathf.FloorToInt(z / (360f / numSlices));
		if(index < 0 || index >= slices.Count) return 0;

		return slices[index].Number;
	}
	
	List<SpinSample> samples = new List<SpinSample>();

	Image _dragImage;
	public Image dragImage { 
		get {
			if(_dragImage == null) _dragImage = GetComponent<Image>();
			return _dragImage;
		}
	}

	public float TokenRadius { get { return tokenRadius; } }

	void Awake() {
		rTrans = transform as RectTransform;
		canvas = GetComponentInParent<Canvas>();

		Application.targetFrameRate = 60;
	}

	void OnEnable() {
		Unlock();

		//RefreshSettings();
		//OptionsScreen.RefreshSettings += RefreshSettings;

		RefreshSlices();
	}

	void OnDisable() {
		//OptionsScreen.RefreshSettings -= RefreshSettings;
	}

	bool pegPassed = false;
	float pegBendFactor = 0f;
	AudioSource loopingSource = null;
	void Update() {
		if(locked) {
			StopLoopingSound();
			return;
		}

		if(dragging) {
			//doing this in update instead of OnDrag callback so as to sample from a reasonable deltaTime
 			DragUpdate();
			return;
		}

		bool wasSpinning = angularVelocity != 0f;

		DecayVelocity();

		if(angularVelocity != 0f) {
			UpdateRotation();
		}
		else if(wasSpinning) {
			StopLoopingSound();
			Finished = true;
		}
	}

	void UpdateRotation() {
		pegPassed = false;
		float direction = -Mathf.Sign(angularVelocity);
		pegBendFactor = Mathf.Lerp(pegBendFactor, 0f, 0.25f);
		float angle = MathUtil.SignedVectorAngle(Vector3.up, wheel.localRotation*Vector3.up, Vector3.forward);
		angle = ApplySpin(angle, Time.deltaTime);
		wheel.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);
		
		if(peg != null) peg.localRotation = Quaternion.AngleAxis(pegBendAngle * pegBendFactor * direction, Vector3.forward);
		
		if(pegPassed && pegSound != null && Mathf.Abs(angularVelocity) < pegSlowDownThreshold) {
			//Debug.Log ("frame("+Time.frameCount+") pegSound!");
			float pitchFactor = Mathf.Clamp01(Mathf.Abs(angularVelocity) / pegSlowDownThreshold);
			float volumeFactor = 1f - Mathf.Clamp01(Mathf.Abs(angularVelocity) / pegSlowDownThreshold);
			
			float p = Mathf.Lerp(0.75f, 1.5f, pitchFactor);
			float v = Mathf.Lerp(0f, 1f, volumeFactor);
			
			AudioManager.PlayOneShot(pegSound, 0f, p, v);
		}
		
		if(spinLoopSound != null) {
			if(loopingSource == null) {
				loopingSource = AudioManager.PlayAudioClipLooping(spinLoopSound, 0f, AudioManager.Source.Gameplay);
			}
			float pitchFactor = Mathf.Abs(angularVelocity) / (pegSlowDownThreshold * 2f);
			float volumeFactor = Mathf.Abs(angularVelocity) / pegSlowDownThreshold;
			loopingSource.pitch = Mathf.Lerp(0.5f, 2f, pitchFactor);
			loopingSource.volume = Mathf.Lerp(0f, 1f, volumeFactor);
		}
	}
	
	void StopLoopingSound() {
		if(spinLoopSound == null) return;
		if(loopingSource == null) return;
		AudioManager.Stop(spinLoopSound);
		loopingSource = null;
	}
	
	
	void DecayVelocity() {
		if(angularVelocity == 0f) return;

		if(Mathf.Abs(angularVelocity) > maxAngularVelocity) {
			angularVelocity = maxAngularVelocity * Mathf.Sign(angularVelocity);
		}

		float reduction = angularVelocity * dragCoefficient * Time.deltaTime;

		angularVelocity -= reduction;

		if(Mathf.Abs (angularVelocity) <= minAngularVelocity) angularVelocity = 0f;
		if(Mathf.Abs (angularVelocity) <= fadeBelowAngularVelocity) angularVelocity = Mathf.Lerp(Mathf.Abs(angularVelocity), 0f, 0.25f) * Mathf.Sign(angularVelocity);
	}

	//recursively apply spin angle for this frame, with consideration for the effects that pegs have on our
	//angle and velocity
	float ApplySpin(float wheelAngle, float deltaTime) {
		if(angularVelocity == 0f) return wheelAngle;
		if(deltaTime <= 0f) return wheelAngle;

		float remainingArc = angularVelocity * deltaTime;

		if(usePegs) {
			if(Mathf.Abs(angularVelocity) <= pegSlowDownThreshold) {
	
				float pegArc = 360f / numSlices;
				float modArc = nfModulo(wheelAngle + pegDegrees * 0.5f * Mathf.Sign(angularVelocity), pegArc);
				float angleToNextPeg = 0f;
				if(Mathf.Sign(angularVelocity) >= 0) {
					angleToNextPeg = pegArc - modArc;
				}
				else {
					angleToNextPeg = modArc;
				}
				
				//step through each peg traversed and remove angular vel as necessary
				if(angleToNextPeg > 0f && Mathf.Abs(remainingArc) > angleToNextPeg) {
					//remove the time we've spent to get to the peg
					deltaTime -= deltaTime * Mathf.Abs(angleToNextPeg / remainingArc);
					float traversedToPeg = angleToNextPeg * Mathf.Sign(angularVelocity);
					
					//add the angle we've traversed thus far
					wheelAngle += traversedToPeg;
					remainingArc -= traversedToPeg;
					
					//if our energy will get us passed this peg, 
					float pegTraversalCost = pegDegrees * pegTraversalCostFactor;
					float portionOfPegTraversed = Mathf.Clamp01(Mathf.Abs(remainingArc) / pegTraversalCost);

					wheelAngle += portionOfPegTraversed * pegDegrees * Mathf.Sign(angularVelocity);
					deltaTime -= deltaTime * Mathf.Abs(portionOfPegTraversed * pegTraversalCost / remainingArc);

					pegBendFactor = 1f;
					pegPassed = true;

					if(Mathf.Abs(angularVelocity) < pegBounceBelowVel) {
						angularVelocity = -angularVelocity * pegBounceVelFactor;
						//Debug.Log ("frame("+Time.frameCount+") ApplyAngularVelocity bounced by peg at currentAngle("+wheelAngle+") newVelocity("+angularVelocity+") pegBendFactor("+pegBendFactor+")");
					}
					else {
						//we know we are gonna clear this bad-boy ultimately, so flip the peg all the way
						pegBendFactor = 1f;
						//angularVelocity *= Mathf.Lerp(pegSlowMax, pegSlowMin, Mathf.Abs(angularVelocity) / pegSlowDownThreshold);
						//Debug.Log ("frame("+Time.frameCount+") ApplyAngularVelocity passed peg at currentAngle("+wheelAngle+") newVelocity("+angularVelocity+") pegBendFactor("+pegBendFactor+")");
					}

					return ApplySpin(wheelAngle, deltaTime);
				}
			}
			else {
				pegBendFactor = 1f;
				pegPassed = true;
			}
		}
		
		//add our frame's remaining arc
		wheelAngle += remainingArc;

		return wheelAngle;
	}

	float nfModulo(float a,float b) {
		return a - b * Mathf.FloorToInt(a / b);
	}

	void RefreshSlices() {
	
		numSlices = Mathf.Max(0, numSlices);

		while(slices.Count < numSlices) {
			Transform slice = (Transform)GameObject.Instantiate(slicePrefab);
			slice.SetParent(wheel, false);
			slices.Add(slice.gameObject.GetComponent<PieSlice>());
		}

		while(slices.Count > numSlices) {
			GameObject.Destroy(slices[0].gameObject);
			slices.RemoveAt(0);
		}

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
			float fillAmount = 1f / numSlices;
			float angle = -360f / numSlices;
			for(int i=0;i<numSlices;i++) {
				Color imageColor = imageColors[i % imageColors.Length];
				Color textColor = textColors[i % textColors.Length];
				
				slices[i].Initialize(fillAmount, angle*i, (i % 4) + 1, imageColor, textColor);
				pegs[i].localRotation = Quaternion.AngleAxis(angle*i, Vector3.forward);
			}
		}

		inputField_numSlices.text = numSlices.ToString();
	}

	void RefreshSettings() {
		spinMultiplier = PlayerPrefs.GetFloat("SpinMultiplier", 2f);
		minStartingVelocity = PlayerPrefs.GetFloat("SpinnerMinStartingVelocityDegPS", 360f);
		maxAngularVelocity = PlayerPrefs.GetFloat("SpinnerMaxAngularVelocityDegPS", 3600f);
		minAngularVelocity = PlayerPrefs.GetFloat("SpinnerMinAngularVelocityDegPS", 1f);

		dragCoefficient = PlayerPrefs.GetFloat("DragCoefficient", 0.3f);
		maxSamples = PlayerPrefs.GetInt("MaxDragSamples", 5);
		pegSlowDownThreshold = PlayerPrefs.GetFloat("SpinnerPegsSlowDownThreshold", 360f);

		usePegs = pegSlowDownThreshold != 0f;
	}

	public void OnPointerDown(PointerEventData eventData) {
		if(locked) return;
		if(angularVelocity != 0f) return;
		
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, eventData.position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out lastTouchPos);
		dragging = true;
		angularVelocity = 0f;
		lastTime = Time.time;
		samples.Clear();
		//Debug.Log ("SpinWheel OnPointerDown");
	}
	
	public void OnPointerUp(PointerEventData eventData) {
		if(locked) return;
		if(angularVelocity != 0f) return;

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

		if(Mathf.Abs (angularVelocity) < minStartingVelocity) {
			angularVelocity = minStartingVelocity * Mathf.Sign(angularVelocity);
		}

		//string debugString = "SpinWheel OnPointerUp angularVelocity("+angularVelocity+") samples["+samples.Count+"] { ";
		//for(int i=0;i<samples.Count;i++) {
		//	debugString += (samples[i].deltaAngle / samples[i].deltaTime);
		//	if(i<samples.Count-1) debugString += ", ";
		//}
		//debugString += "}";

		//Debug.Log (debugString);

	}

	public void DragUpdate() {

		pegBendFactor = Mathf.Lerp(pegBendFactor, 0f, 0.25f);

		int lastNum = GetCurrentNumber();
		Vector2 touchPos;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, Input.mousePosition, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out touchPos);
		float deltaAngle = MathUtil.SignedVectorAngle(lastTouchPos, touchPos, Vector3.forward);

		wheel.localRotation = Quaternion.AngleAxis(deltaAngle, Vector3.forward) * wheel.localRotation;

		float deltaTime = Time.time - lastTime;
		if(deltaTime > 0f && Mathf.Abs(deltaAngle) > 0f) {
			SpinSample sample = new SpinSample();
			sample.deltaTime = Time.time - lastTime;
			sample.deltaAngle = deltaAngle;
			samples.Add(sample);
			while(samples.Count > maxSamples) samples.RemoveAt(0);

			lastTouchPos = touchPos;
			lastTime = Time.time;

			float velocity = deltaAngle / deltaTime;
	
			int newNum = GetCurrentNumber();
			if(newNum != lastNum && pegSound != null && Mathf.Abs(velocity) < pegSlowDownThreshold) {
				//Debug.Log ("frame("+Time.frameCount+") pegSound!");
				float pitchFactor = Mathf.Clamp01(Mathf.Abs(velocity) / pegSlowDownThreshold);
				float volumeFactor = 1f - Mathf.Clamp01(Mathf.Abs(velocity) / pegSlowDownThreshold);
				
				float p = Mathf.Lerp(0.75f, 1.5f, pitchFactor);
				float v = Mathf.Lerp(0.25f, 1f, volumeFactor);
				
				AudioManager.PlayOneShot(pegSound, 0f, p, v);
				pegBendFactor = 1f;
			}

		}

		float direction = -Mathf.Sign(deltaAngle);
		
		if(peg != null) peg.localRotation = Quaternion.AngleAxis(pegBendAngle * pegBendFactor * direction, Vector3.forward);

		//Debug.Log ("OnDrag delta("+delta+")");
	}

	public void Lock() {
		locked = true;
		angularVelocity = 0f;
		dragImage.enabled = false;
	}

	public void HidePeg() {
		pegBase.gameObject.SetActive(false);
	}

	public void ShowPeg() {
		pegBase.gameObject.SetActive(true);
	}

	public void Unlock() {
		locked = false;
		Finished = false;
		dragImage.enabled = true;
	}

	public void ResetWheel() {
		wheel.localRotation = Quaternion.identity;
		angularVelocity = 0f;
		Finished = false;
	}

	public void SetNumSlices(int num) {
		numSlices = num;
		RefreshSlices();
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
}
