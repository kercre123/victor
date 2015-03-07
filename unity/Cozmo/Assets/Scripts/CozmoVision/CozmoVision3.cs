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

	[SerializeField] protected SelectionBox box;
	[SerializeField] protected Image reticle;

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision3" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			robot = RobotEngineManager.instance.current;

			if( observedObjectsCount > 0 && robot.observedObjects[0].VizRect.width > reticle.rectTransform.rect.width && 
			   robot.observedObjects[0].VizRect.height > reticle.rectTransform.rect.height )
			{
				box.image.gameObject.SetActive( true );

				ObservedObject observedObject = robot.observedObjects[0];

				box.image.rectTransform.sizeDelta = new Vector2( observedObject.VizRect.width, observedObject.VizRect.height );
				box.image.rectTransform.anchoredPosition = new Vector2( observedObject.VizRect.x, -observedObject.VizRect.y );

				box.text.text = "Select " + observedObject.ID;
				robot.selectedObject = observedObject.ID;
			}
			else
			{
				box.image.gameObject.SetActive( false );
				robot.selectedObject = -1;
			}

			for( int i = 0; i < actionButtons.Length; ++i )
			{
				actionButtons[i].button.gameObject.SetActive( ( i == 0 && robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK && robot.selectedObject == -1 ) || robot.selectedObject > -1 );

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
		}
	}
}
