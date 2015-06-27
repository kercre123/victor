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

	public class SpinData {

		float _wheelAngle = 0f;
		public float wheelAngle {
			get {
				return _wheelAngle;
			}		
			set {
				_wheelAngle = value;
				if(!spinWheel.PegTouching(ref myDataRef)) sliceIndex = spinWheel.GetSliceIndexAtAngle(_wheelAngle);
			}		
		}

		public float wheelAngleAtSpinStart = 0f;

		public float angularVel = 0f;
		public float lastTime = 0f;
		public float lastAngle = 0f;
		public float degToPegEnd = 0f;
		public float pegBendDirection = 0f;
		public float sliceCenterAngle { get { return sliceIndex*spinWheel.sliceArc + spinWheel.sliceArc*0.5f; } }

		public int sliceIndex = 0;

		public PieSlice slice { get { return (sliceIndex >= 0 && sliceIndex < spinWheel.slices.Count) ? spinWheel.slices[sliceIndex] : null; } }

		public int pegsHit = 0;

		bool _pegTouching = false;
		public bool pegTouching {
			get { return _pegTouching; }
			set {
//				if(value != _pegTouching) {
//					Debug.Log("frame("+Time.frameCount+") pegTouching = " + value);
//				}
				_pegTouching = value;
			}
		}

		public int pegIndex = -1;
		public float totalTime = 0f;


		SpinWheel spinWheel;
		SpinData myDataRef;

		public SpinData(SpinWheel spinWheel) {
			this.spinWheel = spinWheel;
			this.myDataRef = this;
		}

		public void Copy(SpinData data) {
			wheelAngle = data.wheelAngle;
			wheelAngleAtSpinStart = data.wheelAngleAtSpinStart;
			angularVel = data.angularVel;
			lastTime = data.lastTime;
			lastAngle = data.lastAngle;
			degToPegEnd = data.degToPegEnd;
			pegBendDirection = data.pegBendDirection;
			sliceIndex = data.sliceIndex;
			pegsHit = data.pegsHit;
			pegTouching = data.pegTouching;
			pegIndex = data.pegIndex;
		}

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
	[SerializeField] float pegMaxAngularAccel = 10f;
	[SerializeField] Color[] imageColors = { Color.white, Color.black };
	[SerializeField] Color[] spokeColors = { Color.black, Color.white };
	[SerializeField] Color[] textColors = { Color.black , Color.white };
	[SerializeField] Color[] outlineColors = { Color.black , Color.white };
	[SerializeField] Color finalSliceColor = Color.magenta;
	[SerializeField] Color finalTextColor = Color.cyan;
	[SerializeField] Color finalTextOutlineColor = Color.cyan;
	[SerializeField] Color unfocusedColor = Color.grey;

	[SerializeField] float tokenRadius = 300f;
	[SerializeField] float tokenOuterRadius = 300f;
	[SerializeField] Text textPrediction;

	[SerializeField] GameObject supplentalEffects;

	#endregion

	#region PUBLIC MEMBERS

	public int GetActualNumber() {
		return actualData.slice != null ? actualData.slice.Number : 0;
	}

	public int GetPredictedNumber() {
		return predictedData.slice != null ? predictedData.slice.Number : 0;
	}

	public bool Finished { get; private set; }
	public bool SpinUnderway { get { return actualData.angularVel != 0f && !dragging; } }

	public float Speed { get { return Mathf.Abs(actualData.angularVel / 360f); } }
	public float TokenRadius { get { return tokenRadius; } }
	public float TokenOuterRadius { get { return tokenOuterRadius; } }

	public int PredictedNumber { get; private set; }
	public float PredictedDurationSeconds { get; private set; }
	public float PredictedTotalRotations { get; private set; }

	#endregion

	#region PRIVATE MEMBERS

	RectTransform rTrans = null;
	Canvas canvas = null;
	AudioSource loopingAudioSource = null;


	//touching
	bool locked = false;
	bool allowTouches = false;
	bool dragging = false;
	Vector2 lastTouchPos = Vector2.zero;
	List<SpinSample> samples = new List<SpinSample>();

	List<RectTransform> pegs = new List<RectTransform>();
	List<Image> pegImages = new List<Image>();
	float pegBendFactor = 0f;

	List<PieSlice> slices = new List<PieSlice>();

	//some members need a separate version for prediction logic to occur simultaneously 
	//	(without having to maintain a separate instance of this component just for prediction)
	SpinData actualData;
	float lastActualTimeStamp = -1f;
	Coroutine spinCoroutine;

	SpinData predictedData;
	float maxPredictionTimePerFrame = 0.00833333333f;
	float lastPredictionTimeStamp = -1f;
	float predictionStartStamp = -1f;
	int predictionStartFrame = -1;
	Coroutine predictionCoroutine;

	float sliceArc { get { return 360f / numSlices; } }

	int GetSliceIndexAtAngle(float angle) {
		
		angle = MathUtil.ClampAngle0_360(-angle);
		
		int index = Mathf.FloorToInt(angle / sliceArc);
		if(index < 0 || index >= slices.Count) return 0;
		
		return index;
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

		InitData();
	}

	void OnEnable() {
		//RefreshSettings();
		//OptionsScreen.RefreshSettings += RefreshSettings;
		SetNumSlices(numSlices);
		ResetWheel();

		actualData.sliceIndex = GetSliceIndexAtAngle(actualData.wheelAngle);
		predictedData.sliceIndex = GetSliceIndexAtAngle(predictedData.wheelAngle);
	}

	void OnDisable() {
		//OptionsScreen.RefreshSettings -= RefreshSettings;
		StopLoopingSound();

		if(predictionCoroutine != null) StopCoroutine(predictionCoroutine);
		if(spinCoroutine != null) StopCoroutine(spinCoroutine);
		
	}

//	void FixedUpdate() {
//		//Debug.Log("SpinWheel FixedUpdate");
//
//		if(locked) return;
//		if(dragging) return;
//
//		bool wasSpinning = SpinUnderway;
//
//		DecayVelocity(Time.deltaTime, ref actualData);
//
//		if(SpinUnderway) {
//			SpinUpdate(Time.time, ref actualData);
//		}
//		else if(wasSpinning) {
//			SpinEnd();
//		}
//	}

	void Update() {

		if(dragging) {
			//doing this in update instead of OnDrag callback so as to sample from a reasonable deltaTime
			DragUpdate(Input.mousePosition, Time.time);
		}

		//Debug.Log("SpinWheel FixedUpdate");
		RefreshDisplay(actualData.pegsHit > 0, SpinUnderway);
		actualData.pegsHit = 0;
	}

	#endregion

	#region PRIVATE METHODS

	void SpinStart() {
		Finished = false;
		actualData.totalTime = 0f;
		actualData.wheelAngleAtSpinStart = actualData.wheelAngle;

		//do effect?

		if(predictionCoroutine != null) StopCoroutine(predictionCoroutine);
		if(spinCoroutine != null) StopCoroutine(spinCoroutine);

		predictionCoroutine = StartCoroutine(PredictSpinResults());
		spinCoroutine = StartCoroutine(RefreshActualSpin());
	}

	void SpinUpdate(float time, ref SpinData data) {
		float deltaTime = time - data.lastTime;
		ApplySpin(deltaTime, ref data);
		data.totalTime += deltaTime;
		data.lastTime = time;
		data.lastAngle = data.wheelAngle;
	}

	void SpinEnd() {
		StopLoopingSound();
		Finished = true;

		if(predictionCoroutine != null) StopCoroutine(predictionCoroutine);
		if(spinCoroutine != null) StopCoroutine(spinCoroutine);

		for(int i=0;i<numSlices;i++) {

			Color imageColor = imageColors[i % imageColors.Length];
			Color textColor = textColors[i % textColors.Length];
			Color outlineColor = outlineColors[i % outlineColors.Length];

			if(i == actualData.sliceIndex) {
				imageColor = finalSliceColor;
				textColor = finalTextColor;
				outlineColor = finalTextOutlineColor;
			}


			slices[i].SetColors(imageColor, textColor, outlineColor);
		}
	}

	IEnumerator RefreshActualSpin() {
		Debug.Log(	"frame("+Time.frameCount+") "+
		          "RefreshActualSpin BEGIN "+
		          "wheelAngleAtSpinStart("+actualData.wheelAngleAtSpinStart+") "+
		          "angularVel("+actualData.angularVel+")");
		
		lastActualTimeStamp = Time.realtimeSinceStartup;
		
		float time = 0f;
		while(actualData.angularVel != 0f) {
		
			DecayVelocity(time - actualData.lastTime, ref actualData);
			SpinUpdate(time, ref actualData);
			time += Time.fixedDeltaTime;

			yield return new WaitForSeconds(Time.fixedDeltaTime);
		}
		
		int actual = GetActualNumber();
		float actualDuraction = actualData.totalTime;
		float actualRotations = Mathf.Abs(actualData.wheelAngle - actualData.wheelAngleAtSpinStart) / 360f;

		SpinEnd();

		Debug.Log(	"frame("+Time.frameCount+") "+
		          "RefreshActualSpin "+
		          "Number("+actual+") "+
		          "Duration("+actualDuraction+") "+
		          "Spins("+actualRotations+") "+
		          "wheelAngle("+actualData.wheelAngle+") "+
		          "wheelAngleAtSpinStart("+actualData.wheelAngleAtSpinStart+")");
		
	}

	[SerializeField] float maxPredictionFramePortion = 0.05f;

	IEnumerator PredictSpinResults() {
		predictedData.Copy(actualData);

		Debug.Log(	"frame("+Time.frameCount+") "+
		          "PredictSpinResults BEGIN "+
		          "wheelAngleAtSpinStart("+predictedData.wheelAngleAtSpinStart+") "+
		          "angularVel("+predictedData.angularVel+")");

		//start prediction coroutine that will hopefully not take too long...
		maxPredictionTimePerFrame = Time.fixedDeltaTime * maxPredictionFramePortion;

		predictionStartFrame = Time.frameCount;
		predictionStartStamp = Time.realtimeSinceStartup;
		lastPredictionTimeStamp = predictionStartStamp;

		PredictedNumber = -1;
		PredictedDurationSeconds = -1f;
		PredictedTotalRotations = -1f;
		
		float time = 0f;
		while(predictedData.angularVel != 0f) {
			//do not take too much time per frame
			if((Time.realtimeSinceStartup - lastPredictionTimeStamp) > maxPredictionTimePerFrame) {
				//Debug.Log(	"frame("+Time.frameCount+") realtimeSinceStartup("+Time.realtimeSinceStartup+") PredictSpinResults yield one frame!");
				yield return 0;
				//Debug.Log(	"frame("+Time.frameCount+") realtimeSinceStartup("+Time.realtimeSinceStartup+") PredictSpinResults frame yielded");
				lastPredictionTimeStamp = Time.realtimeSinceStartup;
			}

			DecayVelocity(time - predictedData.lastTime, ref predictedData);
			SpinUpdate(time, ref predictedData);
			time += Time.fixedDeltaTime;
		}

		int predicted = GetPredictedNumber();
		if(textPrediction != null) textPrediction.text = predicted.ToString();

		PredictedNumber = GetPredictedNumber();
		PredictedDurationSeconds = predictedData.totalTime;
		PredictedTotalRotations = Mathf.Abs(predictedData.wheelAngle - predictedData.wheelAngleAtSpinStart) / 360f;

		float totalPredictionTime = Time.realtimeSinceStartup - predictionStartStamp;
		int totalPredictionFrames = Time.frameCount - predictionStartFrame;

		Debug.Log(	"frame("+Time.frameCount+") "+
		          	"PredictSpinResults "+
		          	"Number("+PredictedNumber+") "+
		          	"Duration("+PredictedDurationSeconds+") "+
		          	"Spins("+PredictedTotalRotations+") "+
		          	"wheelAngle("+predictedData.wheelAngle+") "+
		          	"wheelAngleAtSpinStart("+predictedData.wheelAngleAtSpinStart+") "+
		         	"predictionFrames("+totalPredictionFrames+") "+
		          	"predictionTime("+totalPredictionTime+")");

	}

	void RefreshDisplay(bool doPegSound, bool doLoopSound) {

		wheel.localRotation = Quaternion.AngleAxis(actualData.wheelAngle, Vector3.forward);

		if(doPegSound && pegSound != null && Mathf.Abs(actualData.angularVel) < pegSlowDownThreshold) {
			//Debug.Log ("frame("+Time.frameCount+") pegSound!");
			float pitchFactor = Mathf.Clamp01(Mathf.Abs(actualData.angularVel) / pegSlowDownThreshold);
			float volumeFactor = 1f - Mathf.Clamp01(Mathf.Abs(actualData.angularVel) / pegSlowDownThreshold);
			
			float p = Mathf.Lerp(0.75f, 1.5f, pitchFactor);
			float v = Mathf.Lerp(0f, 1f, volumeFactor);
			
			AudioManager.PlayOneShot(pegSound, 0f, p, v);
		}
		
		if(doLoopSound && spinLoopSound != null) {
			if(loopingAudioSource == null) {
				loopingAudioSource = AudioManager.PlayAudioClipLooping(spinLoopSound, 0f, AudioManager.Source.Gameplay);
			}
			float pitchFactor = Mathf.Abs(actualData.angularVel) / (pegSlowDownThreshold * 2f);
			float volumeFactor = Mathf.Abs(actualData.angularVel) / pegSlowDownThreshold;
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
	
	void DecayVelocity(float deltaTime, ref SpinData data) {
		if(data.angularVel == 0f) return;

		if(Mathf.Abs(data.angularVel) > maxAngularVelocity) {
			data.angularVel = maxAngularVelocity * Mathf.Sign(data.angularVel);
		}

		float reduction = data.angularVel * dragCoefficient * deltaTime;

		data.angularVel -= reduction;

		if(Mathf.Abs (data.angularVel) <= minAngularVelocity) data.angularVel = 0f;
		if(Mathf.Abs (data.angularVel) <= fadeBelowAngularVelocity) data.angularVel = Mathf.Lerp(Mathf.Abs(data.angularVel), 0f, 0.25f) * Mathf.Sign(data.angularVel);
	}

	//recursively apply spin angle for this frame, with consideration for the effects that pegs have on our
	//angle and velocity

	void ApplySpin(float deltaTime, ref SpinData data) {
		if(data.angularVel == 0f) return;
		if(deltaTime <= 0f) return;

		bool lastPegTouching = data.pegTouching;

		float deltaAngle = data.angularVel * deltaTime;

		data.wheelAngle += deltaAngle;

		if(Mathf.Abs(data.angularVel) > pegSlowDownThreshold) {
			data.pegsHit++;
			data.pegTouching = false;
		}
		else if(data.pegTouching) {
			float pegRepulsionFactor = Mathf.Clamp01(data.degToPegEnd / pegDegrees) * pegMaxAngularAccel * deltaTime;
			data.angularVel += pegRepulsionFactor;
			//deltaAngle = data.angularVel * deltaTime;
			
			if(!lastPegTouching) data.pegsHit++;
			//if(data.pegsHit > 0) Debug.Log("ApplySpin PegTouching -> data.pegsHit("+data.pegsHit+") lastPegTouching("+lastPegTouching+")");
		}
		else {
			int crossedPegs = PegsCrossed(data.lastAngle, data.wheelAngle+deltaAngle);

			for(int i=0;i<crossedPegs;i++) {
				data.angularVel *= pegSlowDownFactor;
			}

			data.pegsHit += crossedPegs;

			//if(data.pegsHit > 0) Debug.Log("ApplySpin PegsCrossed -> data.pegsHit("+data.pegsHit+")");
			
		}

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
		pegImages.Clear();

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
				pegImages.Add(pegImage);
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
		actualData.angularVel = 0f;
		actualData.lastTime = time;
		samples.Clear();
	}
	
	void DragUpdate(Vector2 position, float time) {
		//Debug.Log ("SpinWheel DragUpdate position("+position+") time("+time+")");		

		int lastNum = GetActualNumber();
		Vector2 touchPos;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out touchPos);
		float deltaAngle = MathUtil.SignedVectorAngle(lastTouchPos, touchPos, Vector3.forward);
		
		actualData.wheelAngle += deltaAngle;

		float deltaTime = time - actualData.lastTime;
		if(deltaTime > 0f) {
			SpinSample sample = new SpinSample();
			sample.deltaTime = deltaTime;
			sample.deltaAngle = deltaAngle;
			samples.Add(sample);
			while(samples.Count > maxSamples) samples.RemoveAt(0);

			actualData.angularVel = deltaAngle / deltaTime;
			
			int newNum = GetActualNumber();
			if(newNum != lastNum) {
				actualData.pegsHit++;
			}
			
		}

		lastTouchPos = touchPos;
		actualData.lastTime = time;
	}
	
	void EndDrag() {
		//Debug.Log ("SpinWheel EndDrag");

		dragging = false;
		actualData.angularVel = 0f;
		if(samples.Count < 2) return;
		
		float totalTime = 0f;
		float totalAngle = 0f;
		
		for(int i=1;i<samples.Count;i++) {
			totalTime += samples[i].deltaTime;
			totalAngle += samples[i].deltaAngle;
		}
		
		if(totalTime == 0f) return;
		
		actualData.angularVel = totalAngle / totalTime;
		actualData.angularVel *= spinMultiplier;
		
		if(Mathf.Abs(actualData.angularVel) < minStartingVelocity) {
			actualData.angularVel = minStartingVelocity * Mathf.Sign(actualData.angularVel);
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
	
	bool PegTouching(ref SpinData data) {

		bool touching = false;

		float angle = MathUtil.ClampAngle0_360(-data.wheelAngle);

		data.pegBendDirection = -Mathf.Sign(MathUtil.AngleDelta(data.sliceCenterAngle, angle));
		//force the wheel peg to an apt angle if it is still within pegDegrees of a slice peg

		data.pegIndex = Mathf.RoundToInt(angle / sliceArc);
		if(data.pegIndex >= numSlices) data.pegIndex = 0;

		float pegAngle = data.pegIndex*sliceArc;
		float halfPeg = pegDegrees * 0.5f;
		float endOfPegAngle = pegAngle + halfPeg * -Mathf.Sign(data.pegBendDirection);
		float degToPeg = MathUtil.AngleDelta(angle, pegAngle);

		data.degToPegEnd = MathUtil.AngleDelta(angle, endOfPegAngle);
		//arc from current wheel peg angle to the end of slice peg's range, if still pressed against peg, set apt angle
		if(Mathf.Abs(degToPeg) < halfPeg) {
			float factor = Mathf.Clamp01(Mathf.Abs(data.degToPegEnd) / pegDegrees);
			factor *= factor;
			factor = 1f - factor;
			pegBendFactor = factor * data.pegBendDirection;	
			touching = true;
		}
		
		for(int i=0;i<numSlices;i++) {
			Color first = (data.pegIndex != i || !touching) ? Color.green : (data.pegBendDirection > 0 ? Color.red : Color.magenta);
			Color second = (data.pegIndex != i || !touching) ? Color.green : (data.pegBendDirection < 0 ? Color.red : Color.magenta);
			
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

		data.pegTouching = touching;

		return touching;
	}

	int PegsCrossed(float start, float end) {
		//invert angles to match slices and pegs

		int startSliceIndex = GetSliceIndexAtAngle(start);
		int endSliceIndex = GetSliceIndexAtAngle(end);

		int diff = Mathf.Abs(startSliceIndex - endSliceIndex);
		int diff2 = numSlices - diff;

		if(diff > diff2) return diff2;

		return diff;
	}

	float lastPegBendFactor = 0f;
	void RefreshPegAngle() {
		//if we've passed a peg, then just throw it full tilt
		if(!actualData.pegTouching && actualData.pegsHit > 0) {
			pegBendFactor = Mathf.Sign(actualData.angularVel);
		}

//		if(actualData.pegTouching) {
//			pegBendFactor = Mathf.Lerp(lastPegBendFactor, pegBendFactor, 0.5f);
//		}

		if(peg != null) peg.localRotation = Quaternion.AngleAxis(pegBendAngle * pegBendFactor, Vector3.forward);
		
		//decay by default to immitate elasticity
		if(!actualData.pegTouching) {
			pegBendFactor = Mathf.Lerp(pegBendFactor, 0f, 0.25f);
		}

		lastPegBendFactor = pegBendFactor;
		//if(Mathf.Abs(pegBendFactor) < 0.1f) pegBendFactor = 0f;
	}
	
	void InitData() {
		if(actualData == null) actualData = new SpinData(this);
		if(predictedData == null) predictedData = new SpinData(this);
	}

	#endregion

	#region PUBLIC METHODS

	public void Lock() {
		InitData();
		//Debug.Log("Lock wheel("+gameObject.name+")");
		locked = true;
		actualData.angularVel = 0f;
		dragImage.enabled = false;
		StopLoopingSound();
	}


	public void Unfocus() {
		pegBase.gameObject.SetActive(false);
		if(supplentalEffects != null) supplentalEffects.SetActive(false);

		for(int i=0;i<slices.Count;i++) {

			if(slices[i] != null) {
				Color imageColor = unfocusedColor;
				Color textColor = unfocusedColor;
				Color outlineColor = unfocusedColor;
				slices[i].SetColors(imageColor, textColor, outlineColor);
			}
		}
		

		for(int i=0;i<pegImages.Count;i++) {
			if(pegImages[i] != null) {
				pegImages[i].color = unfocusedColor;
			}
		}
	}

	public void Focus() {
		pegBase.gameObject.SetActive(true);
		if(supplentalEffects != null) supplentalEffects.SetActive(true);

		for(int i=0;i<slices.Count;i++) {

			if(slices[i] != null) {
				Color imageColor = imageColors[i % imageColors.Length];
				Color textColor = textColors[i % textColors.Length];
				Color outlineColor = outlineColors[i % outlineColors.Length];
				slices[i].SetColors(imageColor, textColor, outlineColor);
			}
		}
		
		for(int i=0;i<pegImages.Count;i++) {
			if(pegImages[i] != null) {
				pegImages[i].color = spokeColors[i % spokeColors.Length];
			}
		}
	}

	public void Unlock() {
		//Debug.Log("Unlock wheel("+gameObject.name+")");
		locked = false;
		allowTouches = true;
		Finished = false;
		dragImage.enabled = true;

		
		for(int i=0;i<numSlices;i++) {
			
			if(slices[i] != null) {
				Color imageColor = imageColors[i % imageColors.Length];
				Color textColor = textColors[i % textColors.Length];
				Color outlineColor = outlineColors[i % outlineColors.Length];
				slices[i].SetColors(imageColor, textColor, outlineColor);
			}
			
			if(pegImages[i] != null) {
				pegImages[i].color = spokeColors[i % spokeColors.Length];
			}
		}
	}

	public void ResetWheel() {
		InitData();
		//Debug.Log("ResetWheel("+gameObject.name+")");
		actualData.angularVel = 0f;
		actualData.wheelAngle = -sliceArc * 0.5f;

		wheel.localRotation = Quaternion.AngleAxis(actualData.wheelAngle, Vector3.forward);

		Finished = false;
		allowTouches = true;
		pegBendFactor = 0f;
		lastPegBendFactor = 0f;
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
