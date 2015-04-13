using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision_SelectObject : CozmoVision
{
	[SerializeField] private RectTransform reticle;

	protected virtual void Update()
	{
		if( actionPanel == null || actionPanel.gameActions == null ) return;

		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null )
		{
			actionPanel.DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		UnselectNonObservedObjects();
		Dings();
		actionPanel.gameActions.SetActionButtons();
		ShowObservedObjects();

		RefreshFade();

		if( observedObjectBoxes.Find( x => x.gameObject.activeSelf ) != null && !robot.isBusy )
		{
			FadeIn();
		}
		else
		{
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

//	protected override void ObservedObjectSeen( ObservedObjectBox box, ObservedObject observedObject )
//	{
//		base.ObservedObjectSeen( box, observedObject );
//
//		ObservedObjectBox1 box1 = box as ObservedObjectBox1;
//
//		if( !box1.IsInside( reticle, image.canvas.worldCamera ) )
//		{
//			box1.gameObject.SetActive( false );
//		}
//	}

	protected override void ShowObservedObjects()
	{
		if( robot == null || robot.pertinentObjects == null ) return;

		for( int i = 0; i < observedObjectBoxes.Count; ++i )
		{
			if( robot.pertinentObjects.Count > i && !robot.isBusy )
			{
				ObservedObjectSeen( observedObjectBoxes[i], robot.pertinentObjects[i] );
			}
			else
			{
				observedObjectBoxes[i].gameObject.SetActive( false );
			}
		}
	}
}
