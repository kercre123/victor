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

	protected bool inReticle
	{
		get
		{
			Vector3[] corners = new Vector3[4];
			reticle.rectTransform.GetWorldCorners( corners );

			/*Debug.Log( "min x " + box.position.x + " " + corners[0].x );
			Debug.Log( "max x " + box.position.x + " " + corners[2].x );
			Debug.Log( "min y " + box.position.y + " " + corners[0].y );
			Debug.Log( "max y " + box.position.y + " " + corners[2].y );*/

			return box.image.rectTransform.rect.width > reticle.rectTransform.rect.width && 
				  	box.image.rectTransform.rect.height > reticle.rectTransform.rect.height &&
					box.position.x > corners[0].x && 
					box.position.x < corners[2].x &&
					box.position.y > corners[0].y && 
					box.position.y < corners[2].y;
		}
	}

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision3" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			robot = RobotEngineManager.instance.current;

			box.image.gameObject.SetActive( false );
			robot.selectedObject = -1;

			for( int i = 0; i < robot.observedObjects.Count; ++i )
			{
				box.image.rectTransform.sizeDelta = new Vector2( robot.observedObjects[i].width, robot.observedObjects[i].height );
				box.image.rectTransform.anchoredPosition = new Vector2( robot.observedObjects[i].topLeft_x, -robot.observedObjects[i].topLeft_y );

				if( inReticle )
				{
					box.image.gameObject.SetActive( true );

					box.text.text = "Select " + robot.observedObjects[i].ID;
					robot.selectedObject = robot.observedObjects[i].ID;
					//reticle.color = Color.red;

					break;
				}
				/*else
				{
					reticle.color = Color.yellow;
				}*/
			}

			SetActionButtons();
		}
	}
}
