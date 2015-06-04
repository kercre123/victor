using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;

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

		if(targetLockReticle != null) targetLockReticle.gameObject.SetActive(false);

		if(actionPanel != null) {
			actionSliderPanel = actionPanel as ActionSliderPanel;
		}
	}

	protected void Update() {
		if(targetLockTimer > 0) targetLockTimer -= Time.deltaTime;

		if(robot == null) return;

		if(GameActions.instance == null) {
			targetLockReticle.gameObject.SetActive(false);
			if(actionPanel != null && actionPanel.gameObject.activeSelf) {
				//Debug.Log("frame("+Time.frameCount+") actionPanel deactivated because GameActions.instance == null" );
				actionPanel.gameObject.SetActive(false);
			}
			return;
		}

		if(actionPanel != null && !actionPanel.gameObject.activeSelf) {
			//Debug.Log("frame("+Time.frameCount+") actionPanel activated because GameActions.instance != null" );
			actionPanel.gameObject.SetActive(true);
		}

		ShowObservedObjects();
		RefreshFade();
		AcquireTarget();
		GameActions.instance.SetActionButtons(true);

		if(targetLockReticle != null) targetLockReticle.gameObject.SetActive(false);
		if(targetLockAndMarkersVisibleReticle != null) targetLockAndMarkersVisibleReticle.gameObject.SetActive(false);

		if(!robot.searching) {
			targetLockTimer = 0f;
			//Debug.Log("frame("+Time.frameCount+") robot.selectedObjects.Clear();" );
			robot.selectedObjects.Clear();

			robot.targetLockedObject = null;

			FadeOut();

			return;
		}

		FadeIn();

		if(robot.headTrackingObject == null || !robot.headTrackingObject.MarkersVisible) robot.SetHeadAngle(0f);

		if(robot.targetLockedObject != null) {
			RectTransform targetLockTransform = robot.targetLockedObject.MarkersVisible ? targetLockAndMarkersVisibleReticle : targetLockReticle; 
			if(targetLockTransform != null) {
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
		//Debug.Log("CozmoVision4.Update_2 selectedObjects("+robot.selectedObjects.Count+") isBusy("+robot.isBusy+")");
	}

	public void AcquireTarget()
	{
		bool targetingPropInHand = robot.selectedObjects.Count > 0 && robot.carryingObject != null && 
			robot.selectedObjects.Find(x => x == robot.carryingObject) != null;
		bool alreadyHasTarget = robot.pertinentObjects.Count > 0 && robot.targetLockedObject != null && 
			robot.pertinentObjects.Find(x => x == robot.targetLockedObject) != null;
		
		if(targetingPropInHand || !alreadyHasTarget) {
			//Debug.Log("AcquireTarget targetingPropInHand("+targetingPropInHand+") alreadyHasTarget("+alreadyHasTarget+")");
			robot.selectedObjects.Clear();
			robot.targetLockedObject = null;
		}
		
		ObservedObject best = robot.targetLockedObject;
		
		if(robot.selectedObjects.Count == 0) {
			//ObservedObject nearest = null;
			ObservedObject mostFacing = null;
			
			float bestDistFromCoz = float.MaxValue;
			float bestAngleFromCoz = float.MaxValue;
			Vector2 forward = robot.Forward;
			
			for(int i=0; i<robot.pertinentObjects.Count; i++) {
				if(robot.carryingObject == robot.pertinentObjects[i]) continue;
				Vector2 atTarget = robot.pertinentObjects[i].WorldPosition - robot.WorldPosition;
				
				float angleFromCoz = Vector2.Angle(forward, atTarget);
				if(angleFromCoz > 90f) continue;
				
				float distFromCoz = atTarget.sqrMagnitude;
				if(distFromCoz < bestDistFromCoz) {
					bestDistFromCoz = distFromCoz;
					//nearest = robot.pertinentObjects[i];
				}
				
				if(angleFromCoz < bestAngleFromCoz) {
					bestAngleFromCoz = angleFromCoz;
					mostFacing = robot.pertinentObjects[i];
				}
			}
			
			best = mostFacing;
			/*if(nearest != null && nearest != best) {
				Debug.Log("AcquireTarget found nearer object than the one closest to center view.");
				float dist1 = (mostFacing.WorldPosition - robot.WorldPosition).sqrMagnitude;
				if(bestDistFromCoz < dist1 * 0.5f) best = nearest;
			}*/
		}

		//Debug.Log("frame("+Time.frameCount+") AcquireTarget robot.selectedObjects.Clear();" );
		robot.selectedObjects.Clear();
		
		if(best != null) robot.selectedObjects.Add(best);
		
		if(robot.selectedObjects.Count > 0) {
			//find any other objects in a 'stack' with our selected
			for(int i=0; i<robot.pertinentObjects.Count; i++) {
				if(best == robot.pertinentObjects[i])
					continue;
				if(robot.carryingObject == robot.pertinentObjects[i])
					continue;
				
				float dist = Vector2.Distance((Vector2)robot.pertinentObjects[i].WorldPosition, (Vector2)best.WorldPosition);
				if(dist > best.Size.x * 0.5f) {
					//Debug.Log("AcquireTarget rejecting " + robot.pertinentObjects[i].ID +" because it is dist("+dist+") mm from best("+best.ID+") robot.carryingObjectID("+robot.carryingObjectID+")");
					continue;
				}
				
				robot.selectedObjects.Add(robot.pertinentObjects[i]);
			}
			
			//sort selected from ground up
			robot.selectedObjects.Sort(( obj1, obj2 ) => {
				return obj1.WorldPosition.z.CompareTo(obj2.WorldPosition.z);   
			});
			
			if(robot.targetLockedObject == null) {

				if(robot.selectedObjects[0].MarkersVisible) {
					robot.targetLockedObject = robot.selectedObjects[0];
				}
				else {
					Vector2 atTarget = robot.selectedObjects[0].WorldPosition - robot.WorldPosition;
					float angle = Vector2.Angle(robot.Forward, atTarget);
					if(angle < 45f) {
						robot.targetLockedObject = robot.selectedObjects[0];
					}
				}
			}
		}
		//Debug.Log("frame("+Time.frameCount+") AcquireTarget targets(" + robot.selectedObjects.Count + ") from pertinentObjects("+robot.pertinentObjects.Count+")");
	}
}
