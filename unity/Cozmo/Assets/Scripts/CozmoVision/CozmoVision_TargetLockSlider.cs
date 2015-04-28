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

		if(!robot.isBusy && !actionSliderPanel.actionSlider.currentAction.doActionOnRelease) actionSliderPanel.actionSlider.currentAction.button.onClick.Invoke();

		if(targetLockReticle != null) {
			targetLockReticle.gameObject.SetActive(robot.targetLockedObject != null);

			if(robot.targetLockedObject != null) {

				float w = imageRectTrans.sizeDelta.x;
				float h = imageRectTrans.sizeDelta.y;
				ObservedObject lockedObject = robot.targetLockedObject;

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
}
