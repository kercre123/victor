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

			if( observedObjectsCount > 0 && robot.observedObjects[0].width > reticle.rectTransform.rect.width && 
			    robot.observedObjects[0].height > reticle.rectTransform.rect.height )
			{
				box.image.gameObject.SetActive( true );

				ObservedObject observedObject = robot.observedObjects[0];

				box.image.rectTransform.sizeDelta = new Vector2( observedObject.width, observedObject.height );
				box.image.rectTransform.anchoredPosition = new Vector2( observedObject.topLeft_x, -observedObject.topLeft_y );

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
				actionButtons[i].gameObject.SetActive( robot.selectedObject > -1 );
			}
		}
	}
}
