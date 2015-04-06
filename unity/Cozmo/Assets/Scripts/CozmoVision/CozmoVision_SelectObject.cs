using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision_SelectObject : CozmoVision
{
	[SerializeField] private RectTransform reticle;

	private List<ObservedObject> observedObjects;

	protected override void Awake()
	{
		base.Awake();

		observedObjects = new List<ObservedObject>();
	}

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
		if(observedObjects.Count > 0 && !robot.isBusy) {
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

		observedObjects.Clear();
		observedObjects.AddRange( robot.markersVisibleObjects );

		for( int i = 0; i < observedObjectBoxes.Count; ++i )
		{
			if( observedObjects.Count > i && !robot.isBusy )
			{
				ObservedObjectBox1 box = observedObjectBoxes[i] as ObservedObjectBox1;

				if( box.IsInside( reticle, image.canvas.worldCamera ) )
				{
					ObservedObjectSeen( observedObjectBoxes[i], observedObjects[i] );
				}
				else
				{
					observedObjectBoxes[i].gameObject.SetActive( false );
					observedObjects.RemoveAt( i-- );
				}
			}
			else
			{
				observedObjectBoxes[i].gameObject.SetActive( false );
			}
		}
	}
}
