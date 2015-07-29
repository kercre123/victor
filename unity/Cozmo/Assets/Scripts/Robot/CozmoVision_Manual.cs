using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// quick hack-up to let Hanns play with cozmo over the weekend with manual lift control
/// </summary>
public class CozmoVision_Manual : CozmoVision
{
	ManualLiftSliderPanel manualLiftSliderPanel = null;

	protected override void Awake()
	{
		base.Awake();

		if(actionPanel != null)
		{
			manualLiftSliderPanel = actionPanel as ManualLiftSliderPanel;
		}
	}

	protected override void OnEnable()
	{
		base.OnEnable();

		//reset our lifter to our current height
		if(robot != null && manualLiftSliderPanel != null)
		{
			manualLiftSliderPanel.slider.onValueChanged.RemoveListener(LiftSliderChanged);
			manualLiftSliderPanel.slider.value = Mathf.Lerp(0f, 1f, (robot.liftHeight_mm - CozmoUtil.MIN_LIFT_HEIGHT_MM) / (CozmoUtil.MAX_LIFT_HEIGHT_MM - CozmoUtil.MIN_LIFT_HEIGHT_MM));
			manualLiftSliderPanel.slider.onValueChanged.AddListener(LiftSliderChanged);
		}
	}

	void LiftSliderChanged(float val)
	{
		if(robot == null) return;

		robot.SetLiftHeight(val);
	}

}
