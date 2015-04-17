using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision_Manual : CozmoVision
{
	ManualLiftSliderPanel manualLiftSliderPanel = null;

	protected override void Awake() {
		base.Awake();

		if(actionPanel != null) {
			manualLiftSliderPanel = actionPanel as ManualLiftSliderPanel;
		}
	}

	protected override void OnEnable() {
		base.OnEnable();

		robot = RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null;

		//reset our lifter to our current height
		if(robot != null && manualLiftSliderPanel != null) {
			manualLiftSliderPanel.slider.onValueChanged.RemoveListener(LiftSliderChanged);
			manualLiftSliderPanel.slider.value = Mathf.Lerp(0f, 1f, (robot.liftHeight_mm - CozmoUtil.MIN_LIFT_HEIGHT_MM) / (CozmoUtil.MAX_LIFT_HEIGHT_MM - CozmoUtil.MIN_LIFT_HEIGHT_MM));
			manualLiftSliderPanel.slider.onValueChanged.AddListener(LiftSliderChanged);
		}
	}

	void LiftSliderChanged(float val) {
		if(RobotEngineManager.instance == null) return;
		if(RobotEngineManager.instance.current == null) return;
		robot = RobotEngineManager.instance.current;

		float liftHeight = Mathf.Lerp(CozmoUtil.MIN_LIFT_HEIGHT_MM, CozmoUtil.MAX_LIFT_HEIGHT_MM, val);
		robot.SetLiftHeight(liftHeight);
	}

}
