using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// ladies and gentlemen, we have a winner!
/// this is our current preferred style of vision/targeting/ai-assisted action submission paradigm
/// </summary>
public class CozmoVision_TargetLockSlider : CozmoVision {

  [SerializeField] RectTransform targetLockReticle = null;
  [SerializeField] RectTransform targetLockAndMarkersVisibleReticle = null;
  [SerializeField] float targetLockReticleScale = 1.1f;

  ActionSliderPanel actionSliderPanel = null;
  float targetLockTimer = 0f;

  protected override void Reset(DisconnectionReason reason = DisconnectionReason.None) {
    base.Reset(reason);

    targetLockTimer = 0f;
  }

  protected override void Awake() {
    base.Awake();

    if (targetLockReticle != null)
      targetLockReticle.gameObject.SetActive(false);

    if (actionPanel != null) {
      actionSliderPanel = actionPanel as ActionSliderPanel;
    }
  }

  protected void Update() {
    if (targetLockTimer > 0)
      targetLockTimer -= Time.deltaTime;

    if (robot == null)
      return;

    if (GameActions.instance == null) {
      targetLockReticle.gameObject.SetActive(false);
      if (actionPanel != null && actionPanel.gameObject.activeSelf) {
        actionPanel.gameObject.SetActive(false);
      }
      return;
    }

    if (actionPanel != null && !actionPanel.gameObject.activeSelf) {
      actionPanel.gameObject.SetActive(true);
    }

    ShowObservedObjects();
    RefreshFade();
    AcquireTarget();
    GameActions.instance.SetActionButtons(true);

    if (targetLockReticle != null)
      targetLockReticle.gameObject.SetActive(false);
    if (targetLockAndMarkersVisibleReticle != null)
      targetLockAndMarkersVisibleReticle.gameObject.SetActive(false);

    if (!robot.searching) {
      targetLockTimer = 0f;
      robot.seenObjects.Clear();

      robot.targetLockedObject = null;

      FadeOut();

      return;
    }

    FadeIn();

    if (robot.headTrackingObject == null || !robot.headTrackingObject.MarkersVisible)
      robot.SetHeadAngle(0f);

    if (robot.targetLockedObject != null) {
      RectTransform targetLockTransform = robot.targetLockedObject.MarkersVisible ? targetLockAndMarkersVisibleReticle : targetLockReticle; 
      if (targetLockTransform != null) {
        targetLockTransform.gameObject.SetActive(true);
        float w = imageRectTrans.sizeDelta.x;
        float h = imageRectTrans.sizeDelta.y;
        ObservedObject lockedObject = robot.targetLockedObject;

        float lockX = (lockedObject.VizRect.center.x / NativeResolution.x) * w;
        float lockY = (lockedObject.VizRect.center.y / NativeResolution.y) * h;
        float lockW = (lockedObject.VizRect.width / NativeResolution.x) * w;
        float lockH = (lockedObject.VizRect.height / NativeResolution.y) * h;
        
        targetLockTransform.sizeDelta = new Vector2(lockW, lockH) * targetLockReticleScale;
        targetLockTransform.anchoredPosition = new Vector2(lockX, -lockY);
      }
    }
  }

  public void AcquireTarget() {
    bool targetingPropInHand = robot.seenObjects.Count > 0 && robot.carryingObject != null &&
                               robot.seenObjects.Find(x => x == robot.carryingObject) != null;
    bool alreadyHasTarget = robot.pertinentObjects.Count > 0 && robot.targetLockedObject != null &&
                            robot.pertinentObjects.Find(x => x == robot.targetLockedObject) != null;
    
    if (targetingPropInHand || !alreadyHasTarget) {
      robot.seenObjects.Clear();
      robot.targetLockedObject = null;
    }
    
    ObservedObject best = robot.targetLockedObject;
    
    if (robot.seenObjects.Count == 0) {
      for (int i = 0; i < robot.pertinentObjects.Count; i++) {
        float targetScore = robot.pertinentObjects[i].targetingScore;
        if (targetScore > -1 && (best == null || best.targetingScore < targetScore))
          best = robot.pertinentObjects[i];
      }
    }

    robot.seenObjects.Clear();
    
    if (best != null) {
      robot.seenObjects.Add(best);
      //find any other objects in a 'stack' with our selected
      for (int i = 0; i < robot.pertinentObjects.Count; i++) {
        if (best == robot.pertinentObjects[i] || robot.carryingObject == robot.pertinentObjects[i])
          continue;

        float dist = Vector2.Distance((Vector2)robot.pertinentObjects[i].WorldPosition, (Vector2)best.WorldPosition);
        if (dist > best.Size.x * 0.5f) {
          continue;
        }
        
        robot.seenObjects.Add(robot.pertinentObjects[i]);
      }
      
      //sort selected from ground up
      robot.seenObjects.Sort(( obj1, obj2) => {
        return obj1.WorldPosition.z.CompareTo(obj2.WorldPosition.z);   
      });
      
      if (robot.targetLockedObject == null)
        robot.targetLockedObject = robot.seenObjects[0];
    }
  }
}
