using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	protected void Update()
	{
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null )
		{
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		for( int i = 0; i < robot.selectedObjects.Count; ++i )
		{
			if( robot.observedObjects.Find( x => x.ID == robot.selectedObjects[i].ID ) == null )
			{
				robot.selectedObjects.RemoveAt( i-- );
			}
		}

		Dings();
		SetActionButtons();
		ShowObservedObjects();
	}

	protected override void ShowObservedObjects()
	{
		if(robot == null) return;
		if(robot.observedObjects == null) return;

		for( int i = 0; i < maxObservedObjects; ++i )
		{
			if( observedObjectsCount > i && !robot.isBusy )
			{
				ObservedObjectSeen( observedObjectBoxes[i], robot.observedObjects[i] );
			}
			else
			{
				observedObjectBoxes[i].image.gameObject.SetActive( false );
			}
		}
	}
}
