using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision_TargetLockSlider : CozmoVision {

	[SerializeField] RectTransform targetLockReticle = null;
	[SerializeField] float targetLockReticleScale = 1.1f;

	ActionSliderPanel actionSliderPanel = null;
	float targetLockTimer = 0f;

	protected override void Reset( DisconnectionReason reason = DisconnectionReason.None ) {
		base.Reset( reason );

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

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			return;
		}

		robot = RobotEngineManager.instance.current;

		ShowObservedObjects();
		RefreshFade();

		if(!actionSliderPanel.Engaged) {

			targetLockTimer = 0f;
			//Debug.Log("frame("+Time.frameCount+") robot.selectedObjects.Clear();" );
			//robot.selectedObjects.Clear();

			if(targetLockReticle != null) targetLockReticle.gameObject.SetActive(false);

			FadeOut();

			return;
		}

		FadeIn();

		bool targetingPropInHand = robot.selectedObjects.Count > 0 && robot.selectedObjects.Find( x => x.ID == robot.carryingObjectID) != null;
		if((targetingPropInHand || robot.selectedObjects.Count == 0) && !robot.isBusy) AcquireTarget();

		if(targetLockReticle != null) {
			targetLockReticle.gameObject.SetActive(robot.selectedObjects.Count > 0);

			if(robot.selectedObjects.Count > 0) {

				float w = imageRectTrans.sizeDelta.x;
				float h = imageRectTrans.sizeDelta.y;
				ObservedObject lockedObject = robot.selectedObjects[0];

				float lockX = (lockedObject.VizRect.center.x / NativeResolution.x) * w;
				float lockY = (lockedObject.VizRect.center.y / NativeResolution.y) * h;
				float lockW = (lockedObject.VizRect.width / NativeResolution.x) * w;
				float lockH = (lockedObject.VizRect.height / NativeResolution.y) * h;
				
				targetLockReticle.sizeDelta = new Vector2( lockW, lockH ) * targetLockReticleScale;
				targetLockReticle.anchoredPosition = new Vector2( lockX, -lockY );
			}
		}
		//Debug.Log("CozmoVision4.Update_2 selectedObjects("+robot.selectedObjects.Count+") isBusy("+robot.isBusy+")");
	}

	private void AcquireTarget() {
		ObservedObject nearest = null;
		ObservedObject mostFacing = null;

		float bestDistFromCoz = float.MaxValue;
		float bestAngleFromCoz = float.MaxValue;
		Vector2 forward = robot.Forward;

		for(int i=0; i<pertinentObjects.Count; i++) {
			if(robot.carryingObjectID == pertinentObjects[i].ID) continue;
			Vector2 atTarget = pertinentObjects[i].WorldPosition - robot.WorldPosition;

			float angleFromCoz = Vector2.Angle(forward, atTarget);
			if(angleFromCoz > 90f) continue;

			float distFromCoz = atTarget.sqrMagnitude;
			if(distFromCoz < bestDistFromCoz) {
				bestDistFromCoz = distFromCoz;
				nearest = pertinentObjects[i];
			}

			if(angleFromCoz < bestAngleFromCoz) {
				bestAngleFromCoz = angleFromCoz;
				mostFacing = pertinentObjects[i];
			}
		}

		ObservedObject best = mostFacing;
		if(nearest != null && nearest != best) {
			//Debug.Log("AcquireTarget found nearer object than the one closest to center view.");
			//float dist1 = (mostFacing.WorldPosition - robot.WorldPosition).sqrMagnitude;
			//if(bestDistFromCoz < dist1 * 0.5f) best = nearest;
		}

		robot.selectedObjects.Clear();

		if(best != null) {
			robot.selectedObjects.Add(best);

			//find any other objects in a 'stack' with our selected
			for(int i=0; i<pertinentObjects.Count; i++) {
				if(best == pertinentObjects[i])
					continue;
				if(robot.carryingObjectID == pertinentObjects[i].ID)
					continue;

				float dist = Vector2.Distance((Vector2)pertinentObjects[i].WorldPosition, (Vector2)best.WorldPosition);
				if(dist > best.Size.x * 0.5f) {
					//Debug.Log("AcquireTarget rejecting " + robot.knownObjects[i].ID +" because it is dist("+dist+") mm from best("+best.ID+") robot.carryingObjectID("+robot.carryingObjectID+")");
					continue;
				}

				robot.selectedObjects.Add(pertinentObjects[i]);
			}

			//sort selected from ground up
			robot.selectedObjects.Sort(( obj1, obj2 ) => {
				return obj1.WorldPosition.z.CompareTo(obj2.WorldPosition.z);   
			});
		}

		Debug.Log("frame("+Time.frameCount+") AcquireTarget targets(" + robot.selectedObjects.Count + ") from knownObjects("+robot.knownObjects.Count+")");
	}

}
