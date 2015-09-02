using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// this component emulates a Wheel of Fortune style spinner.
///    handles all the ui set up for the pie slices and pegs
///    pre-calculates whole spin duration creating a list of keyframes
///    display interpolates along keyframe list and plays sfx
/// </summary>
public class SpinWheel : MonoBehaviour, IBeginDragHandler, IDragHandler, IEndDragHandler {

  #region NESTED DEFINITIONS

  struct DragSample {
    public float deltaTime;
    public float deltaAngle;
  }

  struct SpinFrame {
    public float time;
    public float angle;
    public float vel;
    public int sliceIndex;
  }

  class SpinData {

    float _wheelAngle = 0f;

    public float wheelAngle {
      get {
        return _wheelAngle;
      }    
      set {
        _wheelAngle = value;
        if (!spinWheel.PegTouch(ref myDataRef))
          sliceIndex = spinWheel.GetSliceIndexAtAngle(_wheelAngle);
      }    
    }

    public float wheelAngleAtSpinStart = 0f;

    public float angularVel = 0f;
    public float lastTime = 0f;
    public float lastAngle = 0f;
    public float degToPegEnd = 0f;
    public int lastPegTouched = 0;

    public float sliceCenterAngle { get { return sliceIndex * spinWheel.sliceArc + spinWheel.sliceArc * 0.5f; } }

    public int sliceIndex = 0;

    public PieSlice slice { get { return (sliceIndex >= 0 && sliceIndex < spinWheel.slices.Count) ? spinWheel.slices[sliceIndex] : null; } }

    public bool pegTouching = false;

    public float totalTime = 0f;
    public float timeAfterLastPeg = 0f;
    public float timeMovingBackwards = 0f;

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
      lastPegTouched = data.lastPegTouched;
      degToPegEnd = data.degToPegEnd;
      sliceIndex = data.sliceIndex;
      pegTouching = data.pegTouching;
      totalTime = data.totalTime;
      timeAfterLastPeg = data.timeAfterLastPeg;
      timeMovingBackwards = data.timeMovingBackwards;
    }

  }

  public enum SpinWheelState {
    LOCKED,
    UNLOCKED,
    AUTOMATED,
    DRAGGING,
    SPINNING,
    FINISHED
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
  [SerializeField] float dragCoefficientUnderPegThreshold = 0.25f;
  [SerializeField] float dragCoefficientOverPegThreshold = 0.5f;
  [SerializeField] int maxSamples = 5;
  [SerializeField] float pegSlowDownThreshold = 360f;
  [SerializeField] float pegSlowDownFactor = 0.95f;
  [SerializeField] float backwardsSlowDownFactor = 0.05f;
  [SerializeField] float pegBounceVelFactor = 0.25f;
  [SerializeField] float pegDegrees = 1f;
  [SerializeField] float pegBendAngle = 30f;
  [SerializeField] float pegMaxAngularAccel = 10f;
  [SerializeField] Color[] imageColors = { Color.white, Color.black };
  [SerializeField] Color[] spokeColors = { Color.black, Color.white };
  [SerializeField] Color[] textColors = { Color.black, Color.white };
  [SerializeField] Color[] outlineColors = { Color.black, Color.white };
  [SerializeField] Color finalSliceColor = Color.magenta;
  [SerializeField] Color finalTextColor = Color.cyan;
  [SerializeField] Color finalTextOutlineColor = Color.cyan;
  [SerializeField] Color unfocusedColor = Color.grey;

  [SerializeField] float tokenRadius = 300f;
  [SerializeField] float tokenOuterRadius = 300f;
  [SerializeField] Text textPrediction;

  [SerializeField] GameObject supplentalEffects;

  [SerializeField] float maxPredictionFramePortion = 0.05f;

  #endregion

  #region PUBLIC MEMBERS

  public int GetDisplayedNumber() {
    return displayData.slice != null ? displayData.slice.Number : 0;
  }

  public int GetPredictedNumber() {
    return predictedData.slice != null ? predictedData.slice.Number : 0;
  }

  public bool Finished { get { return state == SpinWheelState.FINISHED; } }

  public bool Spinning { get { return state == SpinWheelState.SPINNING; } }

  public float SpinStartTime { get; private set; }

  public float Speed { get { return Mathf.Abs(displayData.angularVel / 360f); } }

  public float TokenRadius { get { return tokenRadius; } }

  public float TokenOuterRadius { get { return tokenOuterRadius; } }

  public int PredictedNumber { get; private set; }

  public int PredictedNextNumber { get; private set; }

  public int PredictedPreviousNumber { get; private set; }

  public float PredictedNextNumberWeight { get; private set; }

  public float PredictedPreviousNumberWeight { get; private set; }

  public float TotalDuration { get; private set; }

  public float TimeAfterLastPeg { get; private set; }

  public float TimeMovingBackwards { get; private set; }

  public float TotalRotations { get; private set; }

  public bool SpinClockWise { get { return displayData.angularVel < 0; } }

  #endregion

  #region PRIVATE MEMBERS

  RectTransform rTrans = null;
  Canvas canvas = null;
  AudioSource loopingAudioSource = null;

  Vector2 lastTouchPos = Vector2.zero;
  List<DragSample> samples = new List<DragSample>();

  List<RectTransform> pegs = new List<RectTransform>();
  List<Image> pegImages = new List<Image>();
  float pegBendFactor = 0f;

  List<PieSlice> slices = new List<PieSlice>();

  List<SpinFrame> frames = new List<SpinFrame>();

  SpinData displayData;
  SpinData predictedData;
  float maxPredictionTimePerFrame = 0.00833333333f;
  float lastPredictionTimeStamp = -1f;
  float predictionStartStamp = -1f;
  int predictionStartFrame = -1;
  Coroutine predictionCoroutine;

  float pegBendDirection = 0f;
  int lastSliceIndex = -1;
  bool lastTouching = false;

  float lastPegBendFactor = 0f;

  float sliceArc { get { return 360f / numSlices; } }

  int GetSliceIndexAtAngle(float angle) {
    
    angle = MathUtil.ClampAngle0_360(-angle);
    
    int index = Mathf.FloorToInt(angle / sliceArc);
    if (index < 0 || index >= slices.Count)
      return 0;
    
    return index;
  }

  Image _dragImage;

  Image dragImage { 
    get {
      if (_dragImage == null)
        _dragImage = GetComponent<Image>();
      return _dragImage;
    }
  }

  SpinWheelState state = SpinWheelState.LOCKED;

  AudioSource audioSource;

  #endregion

  #region MONOBEHAVIOR CALLBACKS

  void Awake() {
    rTrans = transform as RectTransform;
    canvas = GetComponentInParent<Canvas>();

    InitData();
    audioSource = GetComponent<AudioSource>();
  }

  void OnEnable() {
    //RefreshSettings();
    //OptionsScreen.RefreshSettings += RefreshSettings;
    SetNumSlices(numSlices);
    ResetWheel();

    displayData.sliceIndex = GetSliceIndexAtAngle(displayData.wheelAngle);
    predictedData.sliceIndex = GetSliceIndexAtAngle(predictedData.wheelAngle);

    lastSliceIndex = displayData.sliceIndex;
  }

  void OnDisable() {
    //OptionsScreen.RefreshSettings -= RefreshSettings;
    StopLoopingSound();
  
  }

  void Update() {

    float angle = displayData.wheelAngle;
    float vel = displayData.angularVel;
    int sliceIndex = displayData.sliceIndex;

    if (state == SpinWheelState.DRAGGING) {
      //doing this in update instead of OnDrag callback so as to sample from a reasonable deltaTime
      DragUpdate(Input.mousePosition, Time.time);
      angle = displayData.wheelAngle;
      vel = displayData.angularVel;
      sliceIndex = displayData.sliceIndex;
    }
    else if (state == SpinWheelState.SPINNING) {
      
      InterpolateSpinValues(ref angle, ref vel, ref sliceIndex);

      if (Mathf.Abs(vel) > 0f && spinLoopSound != null) {
        if (loopingAudioSource == null) {
          loopingAudioSource = AudioManager.PlayAudioClipLooping(spinLoopSound, 0f, AudioManager.Source.Gameplay);
        }
        float pitchFactor = Mathf.Abs(vel) / (pegSlowDownThreshold * 2f);
        float volumeFactor = Mathf.Abs(vel) / pegSlowDownThreshold;
        loopingAudioSource.pitch = Mathf.Lerp(0.5f, 2f, pitchFactor);
        loopingAudioSource.volume = Mathf.Lerp(0f, 1f, volumeFactor);
      }

      if (vel == 0f) {
        SpinEnd();
      }
    }

    displayData.wheelAngle = angle;
    displayData.angularVel = vel;
    displayData.sliceIndex = sliceIndex;

    RefreshWheelDisplay(angle, vel, sliceIndex);
  }

  #endregion

  #region PRIVATE METHODS

  void InterpolateSpinValues(ref float angle, ref float vel, ref int sliceIndex) {
    if (frames.Count == 0)
      return;

    float time = Time.time - SpinStartTime;
    
    SpinFrame? left = frames[0];    
    SpinFrame? right = null;  
    
    for (int i = 1; i < frames.Count; i++) {
      if (frames[i - 1].time < time) {
        left = frames[i - 1];
      }
      
      if (time < frames[i].time) {
        right = frames[i];
        break;
      }
    }

    if (right == null || (right.Value.time - left.Value.time) == 0f) {
      right = frames[frames.Count - 1];
      angle = right.Value.angle;
      vel = right.Value.vel;
      sliceIndex = right.Value.sliceIndex;
      return;
    }

    float factor = Mathf.Clamp01((time - left.Value.time) / (right.Value.time - left.Value.time));
    
    angle = Mathf.Lerp(left.Value.angle, right.Value.angle, factor);
    vel = Mathf.Lerp(left.Value.vel, right.Value.vel, factor);
    sliceIndex = right.Value.sliceIndex;
  }

  void SpinStart() {
    
    pegBendDirection = 0f;
    lastSliceIndex = displayData.sliceIndex;
    lastTouching = false;

    displayData.wheelAngleAtSpinStart = displayData.wheelAngle;

    SpinStartTime = Time.time;
    //do effect?

    bool spinClockwise = displayData.angularVel < 0;

    predictedData.Copy(displayData);

    CalculateSpin(ref predictedData);

    PredictedNumber = GetPredictedNumber();

    int finalIndex = frames[frames.Count - 1].sliceIndex;
    int previousSliceIndex = finalIndex + (spinClockwise ? -1 : 1);
    int nextSliceIndex = finalIndex + (spinClockwise ? 1 : -1);

    if (previousSliceIndex < 0)
      previousSliceIndex = numSlices - 1;
    if (previousSliceIndex >= numSlices)
      previousSliceIndex = 0;

    if (nextSliceIndex < 0)
      nextSliceIndex = numSlices - 1;
    if (nextSliceIndex >= numSlices)
      nextSliceIndex = 0;

    PredictedPreviousNumber = slices[previousSliceIndex].Number;
    PredictedNextNumber = slices[nextSliceIndex].Number;

    TotalDuration = predictedData.totalTime;
    TimeAfterLastPeg = predictedData.timeAfterLastPeg;
    TimeMovingBackwards = predictedData.timeMovingBackwards;
    TotalRotations = Mathf.Abs(predictedData.wheelAngle - predictedData.wheelAngleAtSpinStart) / 360f;

    state = SpinWheelState.SPINNING;

    DAS.Debug("SpinWheel", "SpinStart PredictedNumber(" + PredictedNumber + ") Previous(" + PredictedPreviousNumber + ") Next(" + PredictedNextNumber + ")");

    float finalAngle = MathUtil.ClampAngle0_360(-predictedData.lastAngle) / sliceArc;
    float positionFudgeFactor = finalAngle - Mathf.FloorToInt(finalAngle);
    // calcualte previous and next weights intelligently
    if (previousSliceIndex == predictedData.lastPegTouched || (spinClockwise && finalIndex == predictedData.lastPegTouched)) {
      if (spinClockwise)
        positionFudgeFactor = 1.0f - positionFudgeFactor;
      
      PredictedPreviousNumberWeight = .5f + (positionFudgeFactor / 2.0f);
      PredictedNextNumberWeight = 1.0f - PredictedPreviousNumberWeight;
      DAS.Debug("SpinWheel", "previous was touched last(" + PredictedPreviousNumberWeight + ") nextSliceIndex(" + nextSliceIndex + ") previousSliceIndex(" + previousSliceIndex + ")");
 
    }
    else {
      if (!spinClockwise)
        positionFudgeFactor = 1.0f - positionFudgeFactor;
      PredictedNextNumberWeight = .5f + (positionFudgeFactor / 2.0f);
      PredictedPreviousNumberWeight = 1.0f - PredictedNextNumberWeight;
      DAS.Debug("SpinWheel", "next was touched last (" + PredictedNextNumberWeight + ") previousSliceIndex(" + previousSliceIndex + ") nextSliceIndex(" + nextSliceIndex + ")");
    }

  }

  void SpinUpdate(float time, bool spin_clockwise, ref SpinData data) {
    float deltaTime = time - data.lastTime;

    ApplySpin(deltaTime, spin_clockwise, ref data);
    data.totalTime += deltaTime;
    data.timeAfterLastPeg += deltaTime;
    if (data.pegTouching)
      data.timeAfterLastPeg = 0f;
    if ((spin_clockwise && data.angularVel > 0) || (!spin_clockwise && data.angularVel < 0))
      data.timeMovingBackwards += deltaTime;

    data.lastTime = time;
    data.lastAngle = data.wheelAngle;
  }

  void SpinEnd() {
    StopLoopingSound();


    for (int i = 0; i < numSlices; i++) {

      Color imageColor = unfocusedColor;
      Color textColor = unfocusedColor;
      Color outlineColor = unfocusedColor;

      if (i == displayData.sliceIndex) {
        imageColor = finalSliceColor;
        textColor = finalTextColor;
        outlineColor = finalTextOutlineColor;
      }


      slices[i].SetColors(imageColor, textColor, outlineColor);
    }
    
    state = SpinWheelState.FINISHED;
  }

  void CalculateSpin(ref SpinData data) {
    
    frames.Clear();

    bool spinClockwise = data.angularVel < 0;

    float time = 0f;
    data.lastTime = 0f;
    data.totalTime = 0f;
    data.timeAfterLastPeg = 0f;
    data.timeMovingBackwards = 0f;
    
    SpinFrame first;
    first.time = time;
    first.angle = data.wheelAngle;
    first.vel = data.angularVel;
    first.sliceIndex = data.sliceIndex;
    frames.Add(first);

    while (data.angularVel != 0f) {
      //yield return new WaitForSeconds(Time.fixedDeltaTime);  
      time += Time.fixedDeltaTime;

      DecayVelocity(time, ref data);
      SpinUpdate(time, spinClockwise, ref data);

      SpinFrame frame;
      frame.time = time;
      frame.angle = data.wheelAngle;
      frame.vel = data.angularVel;
      frame.sliceIndex = data.sliceIndex;
      frames.Add(frame);
    }
  
  }

  void RefreshWheelDisplay(float angle, float vel, int sliceIndex) {
    wheel.localRotation = Quaternion.AngleAxis(angle, Vector3.forward);

    RefreshPegDisplay(angle, vel, sliceIndex);
  }

  void StopLoopingSound() {
    if (spinLoopSound == null)
      return;
    if (loopingAudioSource == null)
      return;
    AudioManager.Stop(spinLoopSound);
    loopingAudioSource = null;
  }

  void DecayVelocity(float time, ref SpinData data) {
    float deltaTime = time - data.lastTime;

    if (data.angularVel == 0f)
      return;

    if (Mathf.Abs(data.angularVel) > maxAngularVelocity) {
      data.angularVel = maxAngularVelocity * Mathf.Sign(data.angularVel);
    }

    float dragCoefficient = dragCoefficientUnderPegThreshold;
    if (Mathf.Abs(data.angularVel) > pegSlowDownThreshold) {
      dragCoefficient = dragCoefficientOverPegThreshold;
    }

    float reduction = data.angularVel * dragCoefficient * deltaTime;

    data.angularVel -= reduction;

    if (Mathf.Abs(data.angularVel) <= minAngularVelocity)
      data.angularVel = 0f;
    if (Mathf.Abs(data.angularVel) <= fadeBelowAngularVelocity)
      data.angularVel = Mathf.Lerp(Mathf.Abs(data.angularVel), 0f, 0.25f) * Mathf.Sign(data.angularVel);
  }

  void ApplySpin(float deltaTime, bool spin_clockwise, ref SpinData data) {
    if (data.angularVel == 0f)
      return;
    if (deltaTime <= 0f)
      return;

    float deltaAngle = data.angularVel * deltaTime;

    data.wheelAngle += deltaAngle;

    if (Mathf.Abs(data.angularVel) > pegSlowDownThreshold) {
      data.pegTouching = false;
    }
    else if (data.pegTouching) {
      float pegRepulsionFactor = Mathf.Clamp01(data.degToPegEnd / pegDegrees) * pegMaxAngularAccel * deltaTime;
      data.angularVel += pegRepulsionFactor;
    }
    else {
      int crossedPegs = PegsCrossed(data.lastAngle, data.wheelAngle + deltaAngle);

      for (int i = 0; i < crossedPegs; i++) {
        data.angularVel *= pegSlowDownFactor;
      }
      // apply a slowdown if we are going the opposite way from how we started
      if ((spin_clockwise && data.angularVel > 0) || (!spin_clockwise && data.angularVel < 0)) {
        data.angularVel *= (1 - (backwardsSlowDownFactor * deltaTime));
      }
    }

  }

  void RefreshSliceCount() {

    while (slices.Count < numSlices) {
      Transform slice = (Transform)GameObject.Instantiate(slicePrefab);
      slice.SetParent(wheel, false);
      slices.Add(slice.gameObject.GetComponent<PieSlice>());
    }

    while (slices.Count > numSlices) {
      GameObject.Destroy(slices[0].gameObject);
      slices.RemoveAt(0);
    }

    if (numSlices > 0) {
      float fillAmount = 1f / numSlices;
      for (int i = 0; i < numSlices; i++) {
        Color imageColor = imageColors[i % imageColors.Length];
        Color textColor = textColors[i % textColors.Length];
        Color outlineColor = outlineColors[i % outlineColors.Length];

        slices[i].Initialize(fillAmount, sliceArc * i, sliceArc * 0.5f, (i % 4) + 1, imageColor, textColor, outlineColor);
      }
    }

    inputField_numSlices.text = numSlices.ToString();
  }

  void RefreshPegCount() {
    
    numSlices = Mathf.Max(0, numSlices);
    pegImages.Clear();

    while (pegs.Count < numSlices) {
      Transform peg = (Transform)GameObject.Instantiate(pegPrefab);
      peg.SetParent(wheel, false);
      pegs.Add(peg.transform as RectTransform);
    }
    
    while (pegs.Count > numSlices) {
      GameObject.Destroy(pegs[0].gameObject);
      pegs.RemoveAt(0);
    }
    
    if (numSlices > 0) {
      for (int i = 0; i < numSlices; i++) {
        pegs[i].localRotation = Quaternion.AngleAxis(sliceArc * i, Vector3.forward);
        
        Image pegImage = pegs[i].gameObject.GetComponentInChildren<Image>();
        pegImage.color = spokeColors[i % spokeColors.Length];
        pegImages.Add(pegImage);
      }
    }

  }

  bool PegTouch(ref SpinData data) {

    bool touching = false;

    float angle = MathUtil.ClampAngle0_360(-data.wheelAngle);

    float pegBendDirection = -Mathf.Sign(MathUtil.AngleDelta(data.sliceCenterAngle, angle));
    //force the wheel peg to an apt angle if it is still within pegDegrees of a slice peg

    int pegIndex = Mathf.RoundToInt(angle / sliceArc);
    if (pegIndex >= numSlices)
      pegIndex = 0;

    float pegAngle = pegIndex * sliceArc;
    float halfPeg = pegDegrees * 0.5f;
    float endOfPegAngle = pegAngle + halfPeg * -Mathf.Sign(pegBendDirection);
    float degToPeg = MathUtil.AngleDelta(angle, pegAngle);

    data.degToPegEnd = MathUtil.AngleDelta(angle, endOfPegAngle);
    //arc from current wheel peg angle to the end of slice peg's range, if still pressed against peg, set apt angle
    if (Mathf.Abs(degToPeg) < halfPeg) {
      float factor = Mathf.Clamp01(Mathf.Abs(data.degToPegEnd) / pegDegrees);
      factor *= factor;
      factor = 1f - factor;
      touching = true;
      data.lastPegTouched = pegIndex;
    }
    
//    for(int i=0;i<numSlices;i++) {
//      Debug.DrawRay(transform.position, Quaternion.AngleAxis(i*sliceArc + pegDegrees * 0.5f, Vector3.forward)*wheel.transform.up*1000f, Color.green);
//      Debug.DrawRay(transform.position, Quaternion.AngleAxis(i*sliceArc - pegDegrees * 0.5f, Vector3.forward)*wheel.transform.up*1000f, Color.green);
//    }
    

//    Debug.Log (  "frame("+Time.frameCount+") "+
//                 "PegTouching("+touching+") "+
//          "bend("+pegBendFactor+") "+
//                 "dir("+bendDirection+") "+
//                 "peg("+closestPegIndex+") "+
//                 "angle("+angle+") "+
//                "sliceAngle("+currentSliceCenterAngle+") "+
//                 "sliceIndex("+currentSliceIndex+") "+
//                "endOfPegAngle("+endOfPegAngle+")");

    data.pegTouching = touching;

    return touching;
  }

  int PegsCrossed(float start, float end) {
    //invert angles to match slices and pegs

    int startSliceIndex = GetSliceIndexAtAngle(start);
    int endSliceIndex = GetSliceIndexAtAngle(end);

    int diff = Mathf.Abs(startSliceIndex - endSliceIndex);
    int diff2 = numSlices - diff;

    if (diff > diff2)
      return diff2;

    return diff;
  }

  void RefreshPegDisplay(float angle, float vel, int sliceIndex) {
    
    bool touching = false;
    
    angle = MathUtil.ClampAngle0_360(-angle);

    float sliceCenterAngle = sliceIndex * sliceArc + sliceArc * 0.5f;

    pegBendDirection = -Mathf.Sign(MathUtil.AngleDelta(sliceCenterAngle, angle));
    //force the wheel peg to an apt angle if it is still within pegDegrees of a slice peg

    int pegIndex = Mathf.RoundToInt(angle / sliceArc);

    if (pegIndex >= numSlices)
      pegIndex = 0;
    
    float pegAngle = pegIndex * sliceArc;
    float halfPeg = pegDegrees * 0.5f;
    float endOfPegAngle = pegAngle + halfPeg * -Mathf.Sign(pegBendDirection);
    float degToPeg = MathUtil.AngleDelta(angle, pegAngle);
    
    float degToPegEnd = MathUtil.AngleDelta(angle, endOfPegAngle);
    //arc from current wheel peg angle to the end of slice peg's range, if still pressed against peg, set apt angle
    if (Mathf.Abs(degToPeg) < halfPeg) {
      float factor = Mathf.Clamp01(Mathf.Abs(degToPegEnd) / pegDegrees);
      factor *= factor;
      factor = 1f - factor;
      pegBendFactor = factor * pegBendDirection;  
      touching = true;
    }


    //if we've passed a peg, then just throw it full tilt
    if (!touching && lastSliceIndex != sliceIndex) {
      pegBendFactor = Mathf.Sign(vel);
    }

    if (peg != null)
      peg.localRotation = Quaternion.AngleAxis(pegBendAngle * pegBendFactor, Vector3.forward);
    
    //decay by default to immitate elasticity
    if (!touching) {
      pegBendFactor = Mathf.Lerp(pegBendFactor, 0f, 0.25f);
    }

    lastPegBendFactor = pegBendFactor;

    if (lastSliceIndex != sliceIndex) { //(touching && !lastTouching) || (!touching && 
      if (pegSound != null && Mathf.Abs(vel) < pegSlowDownThreshold) {
        //Debug.Log ("frame("+Time.frameCount+") pegSound!");
        float pitchFactor = Mathf.Clamp01(Mathf.Abs(vel) / pegSlowDownThreshold);
        float volumeFactor = 1f - Mathf.Clamp01(Mathf.Abs(vel) / pegSlowDownThreshold);
        
        float p = Mathf.Lerp(0.75f, 1.5f, pitchFactor);
        float v = Mathf.Lerp(0f, 1f, volumeFactor);
        
        audioSource.pitch = p;
        audioSource.volume = v;
        audioSource.loop = false;
        audioSource.PlayOneShot(pegSound, v);
      }
    }

    lastSliceIndex = sliceIndex;
    lastTouching = touching;
  }

  void InitData() {
    if (displayData == null)
      displayData = new SpinData(this);
    if (predictedData == null)
      predictedData = new SpinData(this);
  }

  #endregion

  #region PUBLIC METHODS

  public void Lock() {
    InitData();
    state = SpinWheelState.LOCKED;
    displayData.angularVel = 0f;
    dragImage.enabled = false;
    StopLoopingSound();
  }

  public void AutomatedMode() {
    InitData();
    state = SpinWheelState.AUTOMATED;
    displayData.angularVel = 0f;
    dragImage.enabled = false;
    StopLoopingSound();
  }

  public void Unfocus() {
    pegBase.gameObject.SetActive(false);
    if (supplentalEffects != null)
      supplentalEffects.SetActive(false);

    for (int i = 0; i < slices.Count; i++) {

      if (slices[i] != null) {
        Color imageColor = unfocusedColor;
        Color textColor = unfocusedColor;
        Color outlineColor = unfocusedColor;
        slices[i].SetColors(imageColor, textColor, outlineColor);
      }
    }
    

    for (int i = 0; i < pegImages.Count; i++) {
      if (pegImages[i] != null) {
        pegImages[i].color = unfocusedColor;
      }
    }
    state = SpinWheelState.LOCKED;
  }

  public void Focus() {
    pegBase.gameObject.SetActive(true);
    if (supplentalEffects != null)
      supplentalEffects.SetActive(true);

    for (int i = 0; i < slices.Count; i++) {

      if (slices[i] != null) {
        Color imageColor = imageColors[i % imageColors.Length];
        Color textColor = textColors[i % textColors.Length];
        Color outlineColor = outlineColors[i % outlineColors.Length];
        slices[i].SetColors(imageColor, textColor, outlineColor);
      }
    }
    
    for (int i = 0; i < pegImages.Count; i++) {
      if (pegImages[i] != null) {
        pegImages[i].color = spokeColors[i % spokeColors.Length];
      }
    }
  }

  public void Unlock() {
    state = SpinWheelState.UNLOCKED;

    dragImage.enabled = true;

    for (int i = 0; i < numSlices; i++) {
      
      if (slices[i] != null) {
        Color imageColor = imageColors[i % imageColors.Length];
        Color textColor = textColors[i % textColors.Length];
        Color outlineColor = outlineColors[i % outlineColors.Length];
        slices[i].SetColors(imageColor, textColor, outlineColor);
      }
      
      if (pegImages[i] != null) {
        pegImages[i].color = spokeColors[i % spokeColors.Length];
      }
    }
  }

  public void ResetWheel() {
    InitData();
    displayData.angularVel = 0f;
    displayData.wheelAngle = -sliceArc * 0.5f;

    wheel.localRotation = Quaternion.AngleAxis(displayData.wheelAngle, Vector3.forward);

    pegBendFactor = 0f;
    lastPegBendFactor = 0f;
  }

  public void SetNumSlices(int num) {
    numSlices = Mathf.Max(0, num);
    RefreshSliceCount();
    RefreshPegCount();
  }

  public void OnSubmitNumSlices() {
    int newSlices;
    
    if (int.TryParse(inputField_numSlices.text, out newSlices)) {
      if (newSlices != numSlices) {
        SetNumSlices(newSlices);
      }
    }
  }

  
  public void DragStart(Vector2 position, float time) {
    RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out lastTouchPos);

    displayData.angularVel = 0f;
    displayData.lastTime = time;
    samples.Clear();
  }

  public void DragUpdate(Vector2 position, float time) {
    Vector2 touchPos;
    RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, position, canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, out touchPos);
    float deltaAngle = MathUtil.SignedVectorAngle(lastTouchPos, touchPos, Vector3.forward);
    
    displayData.wheelAngle += deltaAngle;
    
    float deltaTime = time - displayData.lastTime;
    if (deltaTime > 0f) {
      DragSample sample = new DragSample();
      sample.deltaTime = deltaTime;
      sample.deltaAngle = deltaAngle;
      samples.Add(sample);
      while (samples.Count > maxSamples)
        samples.RemoveAt(0);
      
      displayData.angularVel = deltaAngle / deltaTime;
    }
    
    lastTouchPos = touchPos;
    displayData.lastTime = time;
  }

  public void DragEnd() {
    //Debug.Log ("SpinWheel EndDrag");

    displayData.angularVel = 0f;
    if (samples.Count < 2)
      return;
    
    float totalTime = 0f;
    float totalAngle = 0f;
    
    for (int i = 1; i < samples.Count; i++) {
      totalTime += samples[i].deltaTime;
      totalAngle += samples[i].deltaAngle;
    }
    
    if (totalTime == 0f)
      return;
    
    displayData.angularVel = totalAngle / totalTime;
    displayData.angularVel *= spinMultiplier;
    
    if (Mathf.Abs(displayData.angularVel) < minStartingVelocity) {
      displayData.angularVel = minStartingVelocity * Mathf.Sign(displayData.angularVel);
    }

    SpinStart();
  }

  #endregion

  #region INTERFACE METHODS

  public void OnBeginDrag(PointerEventData eventData) {
    //Debug.Log ("OnBeginDrag");
    if (state != SpinWheelState.UNLOCKED)
      return;
    state = SpinWheelState.DRAGGING;
    DragStart(eventData.position, Time.time);
  }

  public void OnDrag(PointerEventData eventData) {
    //Debug.Log ("OnDrag");
  }

  public void OnEndDrag(PointerEventData eventData) {
    //Debug.Log ("OnEndDrag");
    if (state != SpinWheelState.DRAGGING)
      return;
    
    DragEnd();
  }

  #endregion

}
