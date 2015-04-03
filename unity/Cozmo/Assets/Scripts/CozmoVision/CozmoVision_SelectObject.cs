using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision_SelectObject : CozmoVision
{
	protected virtual void Update()
	{
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null )
		{
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		UnselectNonObservedObjects();
		Dings();
		SetActionButtons();
		ShowObservedObjects();

		RefreshFade();
		if(robot.markersVisibleObjects.Count > 0 && !robot.isBusy) {
			FadeIn();
		}
		else {
			FadeOut();
		}
	}

//	protected override void VisionEnabled()
//	{
//		float alpha = 1f;
//		
//		Color color = image.color;
//		color.a = alpha;
//		image.color = color;
//		
//		color = select;
//		color.a = alpha;
//		select = color;
//		
//		color = selected;
//		color.a = alpha;
//		selected = color;
//	}

	protected override void ShowObservedObjects()
	{
		if( robot == null ) return;

		for( int i = 0; i < observedObjectBoxes.Count; ++i )
		{
			if( robot.markersVisibleObjects.Count > i && !robot.isBusy )
			{
				ObservedObjectSeen( observedObjectBoxes[i], robot.markersVisibleObjects[i] );
			}
			else
			{
				observedObjectBoxes[i].gameObject.SetActive( false );
			}
		}
	}
}
