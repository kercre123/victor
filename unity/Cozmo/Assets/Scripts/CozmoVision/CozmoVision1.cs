using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision1 : CozmoVision
{
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
				}
				else
				{
					selectionBoxes[i].image.gameObject.SetActive( false );
				}
			}

			Queue<SelectionBox> temp = new Queue<SelectionBox>( selectionBoxes );

			for( int i = 0; i < selectionButtons.Length; ++i )
			{
				selectionButtons[i].selectionBox = null;
				selectionButtons[i].gameObject.SetActive( false );
			}

			while( temp.Count > 0 )
			{
				float distance = float.MaxValue;
				SelectionBox selectionBox = temp.Dequeue();
				int index = -1;

				for( int i = 0; i < selectionButtons.Length && i < RobotEngineManager.instance.current.observedObjects.Count; ++i )
				{
					if( selectionButtons[i].selectionBox == null )
					{
						float d = Vector3.Distance( selectionBox.image.transform.position, selectionButtons[i].transform.position );

						if( d < distance )
						{
							distance = d;

							index = i;
						}
					}
				}

				if( index != -1 )
				{
					selectionButtons[index].selectionBox = selectionBox;
					selectionButtons[index].gameObject.SetActive( selectionBox.image.gameObject.activeSelf );
					selectionButtons[index].text.text = selectionBox.text.text;
					selectionButtons[index].line.SetPosition( 0, selectionButtons[index].position );
					selectionButtons[index].line.SetPosition( 1, selectionBox.position );
					selectionButtons[index].line.SetWidth( lineWidth.x, lineWidth.y );
				}
			}
		}
	}
}
