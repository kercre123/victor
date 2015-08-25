using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// handles the unity side of cozmo's camera display as well as managing the actionpanel used to interact with objects he sees
/// </summary>
public class CozmoVision : MonoBehaviour {
  [SerializeField] protected Image image;
  [SerializeField] protected Text text;
  [SerializeField] protected AudioClip newObjectObservedSound;
  [SerializeField] protected AudioClip objectObservedLostSound;
  [SerializeField] protected AudioClip visionActiveLoop;
  [SerializeField] protected float maxLoopingVol = 0.5f;
  [SerializeField] protected AudioClip visionActivateSound;
  [SerializeField] protected float maxVisionStartVol = 0.1f;
  [SerializeField] protected AudioClip visionDeactivateSound;
  [SerializeField] protected float maxVisionStopVol = 0.2f;
  [SerializeField] protected AudioClip selectSound;
  [SerializeField] protected float soundDelay = 2f;
  [SerializeField] protected RectTransform anchorToSnapToSideBar;
  [SerializeField] protected float snapToSideBarScale = 1f;
  [SerializeField] protected RectTransform anchorToCenterOnSideBar;
  [SerializeField] protected RectTransform anchorToScaleOnSmallScreens;
  [SerializeField] protected float scaleOnSmallScreensFactor = 0.5f;
  [SerializeField] protected GameObject observedObjectCanvasPrefab;
  [SerializeField] protected Color selected;
  [SerializeField] protected Color select;

  [SerializeField] protected ActionPanel actionPanel;

  protected RectTransform rTrans;
  protected RectTransform imageRectTrans;
  protected Canvas canvas;
  protected CanvasScaler canvasScaler;
  protected float screenScaleFactor = 1f;
  protected Rect rect;
  protected readonly Vector2 pivot = new Vector2(0.5f, 0.5f);
  protected List<ObservedObjectBox> observedObjectBoxes = new List<ObservedObjectBox>();
  protected GameObject observedObjectCanvas;
  
  private float[] dingTimes = new float[2] { 0f, 0f };
  private static bool imageRequested = false;
  private float fromVol = 0f;
  private bool fade = true;
  private bool fadingOut = false;
  private bool fadingIn = false;
  private float fadeTimer = 0f;
  private float fadeDuration = 1f;
  private float fromAlpha = 0f;
  private static bool dingEnabled = true;

  public bool IsSmallScreen { get; protected set; }

  protected static readonly Vector2 NativeResolution = new Vector2(400f, 296f);

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  protected virtual void Reset(DisconnectionReason reason = DisconnectionReason.None) {
    for (int i = 0; i < dingTimes.Length; ++i) {
      dingTimes[i] = 0f;
    }

    imageRequested = false;
  }

  protected virtual void Awake() {
    rTrans = transform as RectTransform;
    imageRectTrans = image.gameObject.GetComponent<RectTransform>();
    canvas = image.gameObject.GetComponentInParent<Canvas>();
    canvasScaler = canvas.gameObject.GetComponent<CanvasScaler>();

    if (observedObjectCanvasPrefab != null) {
      observedObjectCanvas = (GameObject)GameObject.Instantiate(observedObjectCanvasPrefab);

      Canvas canv = observedObjectCanvas.GetComponent<Canvas>();
      canv.worldCamera = canvas.worldCamera;
      
      observedObjectBoxes.Clear();
      observedObjectBoxes.AddRange(observedObjectCanvas.GetComponentsInChildren<ObservedObjectBox>(true));
      
      foreach (ObservedObjectBox box in observedObjectBoxes)
        box.gameObject.SetActive(false);

    }

    if (actionPanel != null) {
      actionPanel.gameObject.SetActive(false);
    }
  }

  protected virtual void ObservedObjectSeen(ObservedObjectBox box, ObservedObject observedObject) {
    float boxX = (observedObject.VizRect.x / NativeResolution.x) * imageRectTrans.sizeDelta.x;
    float boxY = (observedObject.VizRect.y / NativeResolution.y) * imageRectTrans.sizeDelta.y;
    float boxW = (observedObject.VizRect.width / NativeResolution.x) * imageRectTrans.sizeDelta.x;
    float boxH = (observedObject.VizRect.height / NativeResolution.y) * imageRectTrans.sizeDelta.y;
    
    box.rectTransform.sizeDelta = new Vector2(boxW, boxH);
    box.rectTransform.anchoredPosition = new Vector2(boxX, -boxY);
    
    if (robot.selectedObjects.Find(x => x == observedObject) != null) {
      box.SetColor(selected);
      box.text.text = observedObject.InfoString;
    }
    else {
      box.SetColor(select);
      box.text.text = observedObject.SelectInfoString;
      box.observedObject = observedObject;
    }
    
    box.gameObject.SetActive(true);
  }

  protected void UnselectNonObservedObjects() {
    if (robot == null || robot.pertinentObjects == null)
      return;

    for (int i = 0; i < robot.selectedObjects.Count; ++i) {
      if (robot.pertinentObjects.Find(x => x == robot.selectedObjects[i]) == null) {
        robot.selectedObjects.RemoveAt(i--);
      }
    }

    /*if(robot.selectedObjects.Count == 0) {
      robot.SetHeadAngle();
    }*/
  }

  protected virtual void ShowObservedObjects() {
    if (!image.enabled)
      return;

    if (robot == null || robot.pertinentObjects == null)
      return;
    
    for (int i = 0; i < observedObjectBoxes.Count; ++i) {
      if (robot.pertinentObjects.Count > i) {
        ObservedObjectSeen(observedObjectBoxes[i], robot.pertinentObjects[i]);
      }
      else {
        observedObjectBoxes[i].gameObject.SetActive(false);
      }
    }

  }

  private void RobotImage(Sprite sprite) {
    image.sprite = sprite;
    
    if (text.gameObject.activeSelf) {
      text.gameObject.SetActive(false);
    }
    
    ShowObservedObjects();
  }

  private void RequestImage() {
    if (!imageRequested && robot != null) {
      robot.RequestImage(Anki.Cozmo.CameraResolutionClad.QVGA, Anki.Cozmo.ImageSendMode.Stream);
      robot.SetHeadAngle();
      robot.SetLiftHeight(0f);
      imageRequested = true;
    }
  }

  protected virtual void OnEnable() {
    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.RobotImage += RobotImage;
      RobotEngineManager.instance.DisconnectedFromClient += Reset;

      if (robot != null)
        robot.selectedObjects.Clear();
    }
    
    RequestImage();
    VisionEnabled();

    if (actionPanel != null)
      actionPanel.gameObject.SetActive(true);
  }

  protected virtual void VisionEnabled() {
    fadingIn = false;
    fadingOut = false;
    image.enabled = !OptionsScreen.GetToggleDisableVision();
    fade = !OptionsScreen.GetToggleDisableVisionFade();

    //start at no alpha
    float alpha = fade ? 0f : 1f;

    Color color = image.color;
    color.a = alpha;
    image.color = color;
    
    color = select;
    color.a = alpha;
    select = color;
    
    color = selected;
    color.a = alpha;
    selected = color;

    if (GameLayoutTracker.instance != null) {
      GameLayoutTracker.instance.Show();
    }
  }

  protected void RefreshFade() {
    if (!fadingIn && !fadingOut)
      return;

    fadeTimer += Time.deltaTime;

    float factor = fadeTimer / fadeDuration;

    RefreshLoopingTargetSound(factor);

    float alpha = 0f;
    if (fadingIn) {
      alpha = Mathf.Lerp(fromAlpha, 1f, factor);
    }
    else {
      alpha = Mathf.Lerp(fromAlpha, 0f, factor);
    }

    if (fade || fadingIn) {
      Color color = image.color;
      color.a = alpha;
      image.color = color;
      
      color = select;
      color.a = alpha;
      select = color;
      
      color = selected;
      color.a = alpha;
      selected = color;
    }

    if (factor >= 1f) {
      fadingIn = false;
      fadingOut = false;
    }
  }

  protected void FadeIn() {
    PlayVisionActivateSound();

    if (fadingIn || image.color.a >= 1f || !fade)
      return;

    fadeTimer = 0f;
    fadingIn = true;
    fadingOut = false;

    Color color = image.color;
    fromAlpha = color.a;
    
    color = select;
    color.a = fromAlpha;
    select = color;
    
    color = selected;
    color.a = fromAlpha;
    selected = color;

    if (GameLayoutTracker.instance != null) {
      GameLayoutTracker.instance.Hide();
    }

    if (CozmoStatus.instance != null) {
      CozmoStatus.instance.FadeOut();
    }
  }

  protected void FadeOut() {
    PlayVisionDeactivateSound();

    if (fadingOut || image.color.a < 1f || !fade)
      return;

    fadeTimer = 0f;
    fadingOut = true;
    fadingIn = false;

    Color color = image.color;
    fromAlpha = color.a;
    
    color = select;
    color.a = fromAlpha;
    select = color;
    
    color = selected;
    color.a = fromAlpha;
    selected = color;

    if (GameLayoutTracker.instance != null) {
      GameLayoutTracker.instance.Show();
    }

    if (CozmoStatus.instance != null) {
      CozmoStatus.instance.FadeIn();
    }
  }

  protected void StopLoopingTargetSound() {
    if (AudioManager.CozmoVisionLoop != null)
      AudioManager.CozmoVisionLoop.Stop();
  }

  protected void PlayVisionActivateSound() {
    if (AudioManager.CozmoVisionLoop == null || visionActivateSound == null || AudioManager.CozmoVisionLoop.clip == visionActiveLoop ||
      actionPanel == null || !actionPanel.nonSearchActionAvailable || robot == null || robot.selectedObjects.Count == 0)
      return;

    AudioManager.PlayOneShot(visionActivateSound, 0f, 1f, maxVisionStartVol);

    fromVol = AudioManager.CozmoVisionLoop.volume;
    AudioManager.CozmoVisionLoop.loop = true;
    AudioManager.CozmoVisionLoop.clip = visionActiveLoop;
    AudioManager.CozmoVisionLoop.Play();
  }

  private void PlayVisionDeactivateSound() {
    if (visionDeactivateSound == null || AudioManager.CozmoVisionLoop == null || AudioManager.CozmoVisionLoop.clip != visionActiveLoop)
      return;

    AudioManager.PlayOneShot(visionDeactivateSound, 0f, 1f, maxVisionStopVol);

    AudioManager.CozmoVisionLoop.clip = null;

    fromVol = AudioManager.CozmoVisionLoop.volume;
  }

  protected void RefreshLoopingTargetSound(float factor) {
    if (AudioManager.CozmoVisionLoop == null)
      return;

    if (fadingIn) {
      AudioManager.CozmoVisionLoop.volume = Mathf.Lerp(fromVol, maxLoopingVol, factor);
    }
    else if (fadingOut) {
      AudioManager.CozmoVisionLoop.volume = Mathf.Lerp(fromVol, 0f, factor);
    }

    if (factor >= 1f && fadingOut) {
      AudioManager.CozmoVisionLoop.Stop();
    }

  }

  protected virtual void Dings() {
    if (robot == null || robot.isBusy || robot.selectedObjects.Count > 0)
      return;
      
    if (robot.pertinentObjects.Count > 0/*lastObservedObjects.Count*/) {
      Ding(true);
    }
    /*else if( robot.observedObjects.Count < lastObservedObjects.Count )
    {
      Ding( false );
    }*/
  }

  protected void Ding(bool found) {
    if (newObjectObservedSound == null)
      return;

    if (dingEnabled) {
      if (found) {
        if (dingTimes[0] + soundDelay < Time.time) {
          AudioManager.PlayOneShot(newObjectObservedSound);
        
          dingTimes[0] = Time.time;
        }
      }
    }
  }

  protected virtual void LateUpdate() {
    if (robot != null) {
      robot.lastObservedObjects.Clear();
      robot.lastSelectedObjects.Clear();
      robot.lastMarkersVisibleObjects.Clear();
      
      if (!robot.isBusy) {
        robot.lastObservedObjects.AddRange(robot.observedObjects);
        robot.lastSelectedObjects.AddRange(robot.selectedObjects);
        robot.lastMarkersVisibleObjects.AddRange(robot.markersVisibleObjects);
      }
    }
  }

  protected virtual void OnDisable() {
    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.RobotImage -= RobotImage;
      RobotEngineManager.instance.DisconnectedFromClient -= Reset;
    }
    
    foreach (ObservedObjectBox box in observedObjectBoxes) {
      if (box != null) {
        box.gameObject.SetActive(false);
      }
    }

    StopLoopingTargetSound();

    if (actionPanel != null)
      actionPanel.gameObject.SetActive(false);
    if (GameLayoutTracker.instance != null) {
      GameLayoutTracker.instance.Show();
    }

    if (CozmoStatus.instance != null) {
      CozmoStatus.instance.FadeIn();
    }
  }

  public void Selection(ObservedObject obj) {
    if (robot != null) {
      robot.selectedObjects.Clear();
      robot.selectedObjects.Add(obj);
      robot.TrackToObject(obj);
    }
    
    AudioManager.PlayOneShot(selectSound);
  }

  public static void EnableDing(bool on = true) {
    dingEnabled = on;
  }

}
