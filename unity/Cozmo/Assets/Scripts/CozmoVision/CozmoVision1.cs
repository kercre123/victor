using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision1 : CozmoVision
{
	public class DistancePair
	{
		public float distance;
		public SelectionButton1 button;
		public SelectionBox box;
	}

	[System.Serializable]
	public class SelectionBox
	{
		public Image image;
		public Text text;
		public int ID;
		public SelectionButton1 button { get; set; }

		public Vector3 position
		{
			get
			{
				Vector3 center = Vector3.zero;
				center.z = image.transform.position.z;
				
				Vector3[] corners = new Vector3[4];
				image.rectTransform.GetWorldCorners( corners );
				if( corners == null || corners.Length == 0 )
				{
					return center;
				}
				center = ( corners[0] + corners[2] ) * 0.5f;
				
				return center;
			}
		}
	}
	
	[SerializeField] protected SelectionButton1[] selectionButtons;
	[SerializeField] protected SelectionBox[] selectionBoxes;
	[SerializeField] protected Vector2 lineWidth;

	protected List<DistancePair> distancePairs = new List<DistancePair>();

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision1" ) == 1 );
		
		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			robot = RobotEngineManager.instance.current;

			for( int i = 0; i < actionButtons.Length; ++i )
			{
				// if no object selected or being actioned or holding block
				actionButtons[i].button.gameObject.SetActive( ( i == 0 && robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK ) || robot.selectedObject > -1 );

				if( i == 0 )
				{
					if( robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK )
					{
						if( robot.selectedObject > -1 )
						{
							actionButtons[i].text.text = "Stack";
						}
						else
						{
							actionButtons[i].text.text = "Drop";
						}
					}
					else
					{
						actionButtons[i].text.text = "Pick Up";
					}
				}
			}

			distancePairs.Clear();

			for( int i = 0; i < maxObservedObjects; ++i )
			{
				if( observedObjectsCount > i && robot.selectedObject == -1 )
				{
					ObservedObject observedObject = robot.observedObjects[i];

					selectionBoxes[i].button = null;

					selectionBoxes[i].image.rectTransform.sizeDelta = new Vector2( observedObject.width, observedObject.height );
					selectionBoxes[i].image.rectTransform.anchoredPosition = new Vector2( observedObject.topLeft_x, -observedObject.topLeft_y );
					
					selectionBoxes[i].text.text = "Select " + observedObject.ID;
					selectionBoxes[i].ID = observedObject.ID;
					
					selectionBoxes[i].image.gameObject.SetActive( true );

					for( int j = 0; j < selectionButtons.Length && j < observedObjectsCount; ++j )
					{
						DistancePair dp = new DistancePair();

						dp.box = selectionBoxes[i];
						dp.button = selectionButtons[j];
						dp.distance = Vector3.Distance( dp.box.image.transform.position, dp.button.transform.position );

						distancePairs.Add( dp );
					}
				}
				else
				{
					selectionBoxes[i].image.gameObject.SetActive( false );
				}
			}

			distancePairs.Sort( delegate( DistancePair x, DistancePair y ) { return x.distance.CompareTo( y.distance ); } );

			for( int i = 0; i < selectionButtons.Length; ++i )
			{
				selectionButtons[i].box = null;
				selectionButtons[i].gameObject.SetActive( false );
			}

			for( int i = 0; i < distancePairs.Count; ++i )
			{
				if( distancePairs[i].button.box == null && distancePairs[i].box.button == null )
				{
					distancePairs[i].button.box = distancePairs[i].box;
					distancePairs[i].box.button = distancePairs[i].button;
					distancePairs[i].button.gameObject.SetActive( true );
					distancePairs[i].button.text.text = distancePairs[i].box.text.text;
					distancePairs[i].button.line.SetPosition( 0, distancePairs[i].button.position );
					distancePairs[i].button.line.SetPosition( 1, distancePairs[i].box.position );
					distancePairs[i].button.line.SetWidth( lineWidth.x, lineWidth.y );
				}
			}
		}
	}
}
