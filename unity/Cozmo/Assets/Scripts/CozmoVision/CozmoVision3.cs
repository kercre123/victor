using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision3 : CozmoVision
{
	[System.Serializable]
	public class SelectionBox
	{
		public Image image;
		public Text text;

		private Vector3[] corners;
		public Vector3 position
		{
			get
			{
				Vector3 center = Vector3.zero;
				center.z = image.transform.position.z;

				if( corners == null )
				{
					corners = new Vector3[4];
				}

				image.rectTransform.GetWorldCorners( corners );

				if( corners.Length == 0 )
				{
					return center;
				}

				center = ( corners[0] + corners[2] ) * 0.5f;
				
				return center;
			}
		}
	}

	public struct ActionButtonState
	{
		public bool activeSelf;
		public string text;
	}

	[SerializeField] protected SelectionBox box;
	[SerializeField] protected Image reticle;
	[SerializeField] protected float distance;
	
	protected List<ObservedObject> inReticle = new List<ObservedObject>();

	protected bool isInReticle
	{
		get
		{
			return box.image.rectTransform.rect.center.x > reticle.rectTransform.rect.xMin && 
				   box.image.rectTransform.rect.center.x < reticle.rectTransform.rect.xMax &&
				   box.image.rectTransform.rect.center.y > reticle.rectTransform.rect.yMin && 
				   box.image.rectTransform.rect.center.y < reticle.rectTransform.rect.yMax;
		}
	}

	protected void Update()
	{
		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		box.image.gameObject.SetActive( false );

		if( !robot.isBusy )
		{
			robot.selectedObjects.Clear();
			inReticle.Clear();

			for( int i = 0; i < robot.observedObjects.Count; ++i )
			{
				//Debug.Log( (i + 1) );
				//box.image.gameObject.SetActive( true );
				box.image.rectTransform.sizeDelta = new Vector2( robot.observedObjects[i].VizRect.width, robot.observedObjects[i].VizRect.height );
				box.image.rectTransform.anchoredPosition = new Vector2( robot.observedObjects[i].VizRect.x, -robot.observedObjects[i].VizRect.y );

				/*if( isInReticle )
				{
					Debug.Log( "in reticle" );
				}*/

				/*if( Vector2.Distance( robot.WorldPosition, robot.observedObjects[i].WorldPosition ) < distance )
				{
					Debug.Log( "in distance" );
				}
				else
				{
					Debug.Log( Vector2.Distance( robot.WorldPosition, robot.observedObjects[i].WorldPosition ) );
				}*/

				if( isInReticle && robot.observedObjects[i].Distance < distance )
				{
					inReticle.Add( robot.observedObjects[i] );
				}
			}

			inReticle.Sort( delegate( ObservedObject obj1, ObservedObject obj2 )
			{
				return obj1.Distance.CompareTo( obj2.Distance );   
			} );

			for( int i = 1; i < inReticle.Count; ++i )
			{
				if( Vector2.Distance( inReticle[0].WorldPosition, inReticle[i].WorldPosition ) > inReticle[0].Size.x * 0.5f )
				{
					inReticle.RemoveAt( i-- );
				}
			}

			inReticle.Sort( delegate( ObservedObject obj1, ObservedObject obj2 )
			{
				return obj1.WorldPosition.z.CompareTo( obj2.WorldPosition.z );   
			} );

			if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) ) // if holding a block
			{
				if( inReticle.Count > 0 && inReticle[0].ID != robot.carryingObjectID ) // if can see at least one block
				{
					robot.selectedObjects.Add( inReticle[0].ID );
					
					if( inReticle.Count == 1 )
					{
						RobotEngineManager.instance.TrackHeadToObject( robot.selectedObjects[0], Intro.CurrentRobotID );
					}
				}
				
			}
			else // if not holding a block
			{
				if( inReticle.Count > 0 )
				{
					for( int i = 0; i < 2 && i < inReticle.Count; ++i )
					{
						robot.selectedObjects.Add( inReticle[i].ID );
					}
					
					if( inReticle.Count == 1 )
					{
						RobotEngineManager.instance.TrackHeadToObject( robot.selectedObjects[0], Intro.CurrentRobotID );
					}
				}
				
			}
		}

		SetActionButtons();
		Dings();
	}

	protected override void Dings()
	{
		if( RobotEngineManager.instance != null )
		{
			robot = RobotEngineManager.instance.current;
			
			if( robot == null || robot.isBusy )
			{
				return;
			}

			if( robot.selectedObjects.Count > robot.lastSelectedObjects.Count )
			{
				Ding( true );
				robot.lastSelectedObjects.Clear();
				robot.lastSelectedObjects.AddRange( robot.selectedObjects );
			}
			else if( robot.selectedObjects.Count < robot.lastSelectedObjects.Count )
			{
				Ding( false );
				robot.lastSelectedObjects.Clear();
			}
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
