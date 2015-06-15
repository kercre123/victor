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

	[SerializeField] int numSlices = 8;
	[SerializeField] Transform slicePrefab;

	[SerializeField] InputField inputField_numSlices = null;

	float maxAngularVelocity = 3600f;
	float minAngularVelocity = 1f;
	float minStartingVelocity = 360f;
	float spinMultiplier = 2f;
	float dragCoefficient = 0.3f;
	int maxSamples = 5;
	bool usePegs = true;
	float pegSlowDownThreshold = 360f;
	[SerializeField] float pegBounceThreshold = 5f;
	[SerializeField] float pegBounceVelFactor = 0.25f;
	[SerializeField] float pegVelFactor = 0.25f;
	[SerializeField] float pegDegrees = 1f;
	float pegSlowDownFactor = 0.9f;

	[SerializeField] bool useRigidbodyPhysics = false;

	[SerializeField] Color[] imageColors = { Color.white, Color.black };
	[SerializeField] Color[] textColors = { Color.black , Color.white };

	RectTransform rTrans = null;
	Canvas canvas = null;
	Vector2 lastTouchPos = Vector2.zero;
	float lastTime = 0f;
	float angularVelocity = 0f;
	bool dragging = false;
	bool locked = false;
	
	List<PieSlice> slices = new List<PieSlice>();

	public bool Finished { get; private set; }
	public bool Spinning { get { return angularVelocity != 0f; } }
	public int CurrentNumber { 
		get {
			if(numSlices <= 0) return 0;
			if(slices.Count == 0) return 0;
			
			float z = MathUtil.ClampAngle(wheel.transform.eulerAngles.z);
			if(z < 0) z += 360f;

			int index = Mathf.FloorToInt(z / (360f / numSlices));
			if(index < 0 || index >= slices.Count) return 0;

			return slices[index].Number;
		}
	}
	
	List<SpinSample> samples = new List<SpinSample>();

	void Awake() {
		rTrans = transform as RectTransform;
		canvas = GetComponentInParent<Canvas>();
		Application.targetFrameRate = 60;
	}

	void OnEnable() {
		dragging = false;
		locked = false;
		Finished = false;

		RefreshSettings();
		OptionsScreen.RefreshSettings += RefreshSettings;

		RefreshSlices();
	}

	void OnDisable() {
		OptionsScreen.RefreshSettings -= RefreshSettings;
	}

	void Update() {
		if(locked) return;

		if(dragging) {
			//doing this in update instead of OnDrag callback so as to sample from a reasonable deltaTime
 			DragUpdate();
			return;
		}

		bool wasSpinning = angularVelocity != 0f;

//		if(useRigidbodyPhysics) {
//			angularVelocity = wheel.gameObject
//		}
		DecayVelocity();

		if(angularVelocity != 0f) {
			if(!useRigidbodyPhysics) {
				ApplyAngularVelocity(MathUtil.SignedVectorAngle(Vector3.up, wheel.localRotation*Vector3.up, Vector3.forward));
			}
		}
		else if(wasSpinning) {
			Finished = true;
		}
	}

	void DecayVelocity() {
		if(angularVelocity == 0f) return;

		if(Mathf.Abs(angularVelocity) > maxAngularVelocity) {
			angularVelocity = maxAngularVelocity * Mathf.Sign(angularVelocity);
		}

		float reduction = angularVelocity * dragCoefficient * Time.deltaTime;
		angularVelocity -= reduction;

		if(Mathf.Abs (angularVelocity) <= minAngularVelocity) angularVelocity = 0f;
	}

	void ApplyAngularVelocity(float currentAngle) {
		if(angularVelocity == 0f) return;

		float remainingArc = angularVelocity * Time.deltaTime;

		if(usePegs) {
			if(Mathf.Abs(angularVelocity) <= pegSlowDownThreshold) {
	
				float pegArc = 360f / numSlices;
				float angleToNextPeg = pegArc - Mathf.Abs(currentAngle % pegArc);
		
				//step through each peg traversed and remove angular vel as necessary
				while(Mathf.Abs(remainingArc) > angleToNextPeg) {
					float traversedToPeg = angleToNextPeg * Mathf.Sign(angularVelocity);
					currentAngle += traversedToPeg;
					remainingArc -= traversedToPeg;

					remainingArc -= pegDegrees * pegBounceVelFactor * Mathf.Sign(angularVelocity);

					if(Mathf.Abs(angularVelocity) < pegBounceThreshold) {
						//nudge back to before this peg?
						angularVelocity = -angularVelocity * pegBounceVelFactor;
						remainingArc = -remainingArc;// * pegBounceVelFactor;
						Debug.Log ("frame("+Time.frameCount+") ApplyAngularVelocity bounced off peg at currentAngle("+currentAngle+")");
						//ApplyAngularVelocity(currentAngle);
						break;
					}

					angleToNextPeg = pegArc - Mathf.Abs(currentAngle % pegArc);
					angularVelocity *= pegSlowDownFactor;
				}
			}
		}
		
		//add our frame's remaining arc
		currentAngle += remainingArc;


		wheel.localRotation = Quaternion.AngleAxis(currentAngle, Vector3.forward);
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

		if(numSlices > 0) {
			float fillAmount = 1f / numSlices;
			float angle = -360f / numSlices;
			for(int i=0;i<numSlices;i++) {
				Color imageColor = imageColors[i % imageColors.Length];
				Color textColor = textColors[i % textColors.Length];
				
				slices[i].Initialize(fillAmount, angle*i, (i % 4) + 1, imageColor, textColor);
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
		//pegBounceThreshold = 5f;
		pegSlowDownFactor = PlayerPrefs.GetFloat("SpinnerPegsSlowDownFactor", 0.9f);

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
		}

		//Debug.Log ("OnDrag delta("+delta+")");
	}

	public void Lock() {
		locked = true;
		angularVelocity = 0f;
	}

	public void Unlock() {
		locked = false;
		Finished = false;
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
