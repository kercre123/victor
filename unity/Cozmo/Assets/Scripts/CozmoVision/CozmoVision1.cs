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
		public uint ID;

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
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision" ) == 1 );
		
		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			for( int i = 0; i < actionButtons.Length; ++i )
			{
				// if no object selected or being actioned
				actionButtons[i].gameObject.SetActive( RobotEngineManager.instance.current.selectedObject < uint.MaxValue - 1 );
			}

			distancePairs.Clear();

			for( int i = 0; i < maxBoxes; ++i )
			{
				if( RobotEngineManager.instance.current.observedObjects.Count > i )
				{
					ObservedObject observedObject = RobotEngineManager.instance.current.observedObjects[i];
					
					selectionBoxes[i].image.rectTransform.sizeDelta = new Vector2( observedObject.width, observedObject.height );
					selectionBoxes[i].image.rectTransform.anchoredPosition = new Vector2( observedObject.topLeft_x, -observedObject.topLeft_y );
					
					selectionBoxes[i].text.text = "Select " + observedObject.ID;
					selectionBoxes[i].ID = observedObject.ID;
					
					selectionBoxes[i].image.gameObject.SetActive( true );

					for( int j = 0; j < selectionButtons.Length; ++j )
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

			distancePairs.Sort( delegate( DistancePair x, DistancePair y )
			{
				return x.distance.CompareTo( y.distance );
			});

			for( int i = 0; i < selectionButtons.Length; ++i )
			{
				selectionButtons[i].selectionBox = null;
				selectionButtons[i].gameObject.SetActive( false );
			}

			for( int i = 0; i < selectionBoxes.Length; ++i )
			{
				for( int j = 0; j < distancePairs.Count; ++j )
				{
					if( selectionBoxes[i] == distancePairs[j].box )
					{
						if( distancePairs[j].button.selectionBox == null )
						{
							distancePairs[j].button.selectionBox = distancePairs[j].box;
							distancePairs[j].button.gameObject.SetActive( true );
							distancePairs[j].button.text.text = distancePairs[j].box.text.text;
							distancePairs[j].button.line.SetPosition( 0, distancePairs[j].button.position );
							distancePairs[j].button.line.SetPosition( 1, distancePairs[j].box.position );
							distancePairs[j].button.line.SetWidth( lineWidth.x, lineWidth.y );
						}

						break;
					}
				}
			}
		}
	}
}
