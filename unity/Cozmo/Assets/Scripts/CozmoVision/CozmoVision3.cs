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

	[SerializeField] protected SelectionBox box;
	[SerializeField] protected Image reticle;

	protected Vector3[] reticleCorners;
	protected bool inReticle
	{
		get
		{
			if( reticleCorners == null )
			{
				reticleCorners = new Vector3[4];
				reticle.rectTransform.GetWorldCorners( reticleCorners );
			}

			/*Debug.Log( "min x " + box.position.x + " " + reticleCorners[0].x );
			Debug.Log( "max x " + box.position.x + " " + reticleCorners[2].x );
			Debug.Log( "min y " + box.position.y + " " + reticleCorners[0].y );
			Debug.Log( "max y " + box.position.y + " " + reticleCorners[2].y );*/

			return box.image.rectTransform.rect.width > reticle.rectTransform.rect.width && 
				  	box.image.rectTransform.rect.height > reticle.rectTransform.rect.height &&
					box.position.x > reticleCorners[0].x && 
					box.position.x < reticleCorners[2].x &&
					box.position.y > reticleCorners[0].y && 
					box.position.y < reticleCorners[2].y;
		}
	}

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision3" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			robot = RobotEngineManager.instance.current;

			box.image.gameObject.SetActive( false );

			if( robot.selectedObject > -2 )
			{
				robot.selectedObject = -1;
				//reticle.color = Color.yellow;

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
				}
			}

			SetActionButtons();
		}
	}
}
