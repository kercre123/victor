using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision : MonoBehaviour
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

	[SerializeField] protected Button button;
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;
	[SerializeField] protected SelectionButton[] selectionButtons;
	[SerializeField] protected SelectionBox[] selectionBoxes;
	[SerializeField] protected Button[] actionButtons;
	[SerializeField] protected int maxBoxes;
	[SerializeField] protected Vector2 lineWidth;
	
	protected Rect rect;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			for( int i = 0; i < actionButtons.Length; ++i )
			{
				actionButtons[i].gameObject.SetActive( RobotEngineManager.instance.current.selectedObject != uint.MaxValue );
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

	protected void RobotImage( Texture2D texture )
	{
		if( rect.height != texture.height || rect.width != texture.width )
		{
			rect = new Rect( 0, 0, texture.width, texture.height );
		}

		image.sprite = Sprite.Create( texture, rect, pivot );

		if( text.gameObject.activeSelf )
		{
			text.gameObject.SetActive( false );
		}

		if( button.interactable )
		{
			button.interactable = false;
		}
	}
	
	public void Action()
	{
		Debug.Log( "Action" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.PickAndPlaceObject();

			RobotEngineManager.instance.current.selectedObject = uint.MaxValue;
		}
	}

	public void Cancel()
	{
		Debug.Log( "Cancel" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.selectedObject = uint.MaxValue;
		}
	}

	public void RequestImage()
	{
		Debug.Log( "request image" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RequestImage( Intro.CurrentRobotID );
		}
	}

	protected void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
		}
	}

	protected void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage -= RobotImage;
		}
	}
}
