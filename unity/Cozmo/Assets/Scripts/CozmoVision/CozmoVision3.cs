using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision3 : CozmoVision
{
	public struct ActionButtonState
	{
		public bool activeSelf;
		public string text;
	}

	[SerializeField] protected float maxDistance = 200f;

	protected void Update()
	{
		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		ShowObservedObjects();

		if( !robot.isBusy )
		{
			robot.selectedObjects.Clear();

			/*robot.observedObjects.Sort( ( obj1 ,obj2 ) => // sort by distance from robot
			{
				return obj1.Distance.CompareTo( obj2.Distance );   
			} );*/

			robot.observedObjects.Sort( ( obj1 ,obj2 ) => // sort by most center of view
			{
				return Vector2.Distance( obj1.VizRect.center, image.rectTransform.rect.center ).CompareTo( 
					   Vector2.Distance( obj2.VizRect.center, image.rectTransform.rect.center ) );   
			} );

			for( int i = 0; i < robot.observedObjects.Count && robot.observedObjects.Count > 1; ++i )
			{
				if( robot.observedObjects[i].Distance > maxDistance ) // if multiple in view and too far away
				{
					robot.observedObjects.RemoveAt( i-- );
				}
			}

			for( int i = 1; i < robot.observedObjects.Count; ++i ) // if not on top of selected block, remove
			{
				if( Vector2.Distance( robot.observedObjects[0].WorldPosition, robot.observedObjects[i].WorldPosition ) > robot.observedObjects[0].Size.x * 0.5f )
				{
					robot.observedObjects.RemoveAt( i-- );
				}
			}

			robot.observedObjects.Sort( ( obj1, obj2 ) => { return obj1.WorldPosition.z.CompareTo( obj2.WorldPosition.z ); } );

			if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) ) // if holding a block
			{
				if( robot.observedObjects.Count > 0 && robot.observedObjects[0].ID != robot.carryingObjectID ) // if can see at least one block
				{
					robot.selectedObjects.Add( robot.observedObjects[0] );
					
					if( robot.observedObjects.Count == 1 )
					{
						RobotEngineManager.instance.current.TrackHeadToObject( robot.selectedObjects[0] );
					}
				}
				
			}
			else // if not holding a block
			{
				if( robot.observedObjects.Count > 0 )
				{
					for( int i = 0; i < 2 && i < robot.observedObjects.Count; ++i )
					{
						robot.selectedObjects.Add( robot.observedObjects[i] );
					}
					
					if( robot.observedObjects.Count == 1 )
					{
						RobotEngineManager.instance.current.TrackHeadToObject( robot.selectedObjects[0] );
					}
				}
				
			}
		}

		SetActionButtons();
		Dings();
	}

	protected override void Dings()
	{
		if( robot != null )
		{
			if( !robot.isBusy && robot.selectedObjects.Count > 0/*robot.lastSelectedObjects.Count*/ )
			{
				Ding( true );
			}
			/*else if( robot.selectedObjects.Count < robot.lastSelectedObjects.Count )
			{
				Ding( false );
			}*/
		}
	}

	protected override void SetActionButtons()
	{
		DisableButtons();
		robot = RobotEngineManager.instance.current;
		if(robot == null || robot.isBusy)
			return;
		
		if(robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) {
			if(robot.selectedObjects.Count > 0)
				actionButtons[1].SetMode(ActionButtonMode.STACK);
			actionButtons[0].SetMode(ActionButtonMode.DROP);
		}
		else {
			for(int i = 0; i < robot.selectedObjects.Count; ++i) {
				actionButtons[i].SetMode(ActionButtonMode.PICK_UP, i);
			}
		}
	}

}
