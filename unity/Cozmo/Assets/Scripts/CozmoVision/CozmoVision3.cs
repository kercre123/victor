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
	protected ActionButtonState[] lastActionButtonActiveSelf = new ActionButtonState[2];
	protected Vector3[] reticleCorners;
	protected bool isInReticle
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

			return box.position.x > reticleCorners[0].x && box.position.x < reticleCorners[2].x &&
				   box.position.y > reticleCorners[0].y && box.position.y < reticleCorners[2].y;
		}
	}

	public void Action2()
	{
		if( RobotEngineManager.instance != null )
		{
			if( inReticle.Count > 1 )
			{
				RobotEngineManager.instance.current.selectedObject = inReticle[1].ID;
			}
		}

		Action();
	}

	protected override void Update()
	{
		base.Update();

		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision3" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			robot = RobotEngineManager.instance.current;

			box.image.gameObject.SetActive( false );

			if( robot.selectedObject > -2 )
			{
				robot.selectedObject = -1;
				inReticle.Clear();

				for( int i = 0; i < robot.observedObjects.Count; ++i )
				{
					box.image.rectTransform.sizeDelta = new Vector2( robot.observedObjects[i].VizRect.width, robot.observedObjects[i].VizRect.height );
					box.image.rectTransform.anchoredPosition = new Vector2( robot.observedObjects[i].VizRect.x, -robot.observedObjects[i].VizRect.y );

					if( isInReticle && Vector2.Distance( robot.WorldPosition, robot.observedObjects[i].WorldPosition ) < distance )
					{
						inReticle.Add( robot.observedObjects[i] );
					}
				}

				inReticle.Sort( delegate( ObservedObject obj1, ObservedObject obj2 )
				{
					return obj1.WorldPosition.z.CompareTo( obj2.WorldPosition.z );   
				} );
			}

			if( actionButtons.Length > 1 )
			{
				actionButtons[0].button.gameObject.SetActive( false );
				actionButtons[1].button.gameObject.SetActive( false );

				if( robot.selectedObject > -2 ) // if not in action
				{
					if( robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK ) // if holding a block
					{
						if( inReticle.Count > 0 && inReticle[0].ID != robot.carryingObjectID ) // if can see at least one block
						{
							robot.selectedObject = inReticle[0].ID; // select the block closest to ground
						}

						actionButtons[0].button.gameObject.SetActive( true );
						actionButtons[0].text.text = "Drop " + robot.carryingObjectID;
						actionButtons[1].button.gameObject.SetActive( true );

						if( robot.selectedObject > -1 )
						{
							actionButtons[1].text.text = "Stack " + robot.carryingObjectID + " on " + robot.selectedObject;
						}
						else
						{
							actionButtons[1].text.text = "Change " + robot.carryingObjectID;
						}
					}
					else // if not holding a block
					{
						if( inReticle.Count > 0 )
						{
							actionButtons[0].text.text = "Pick Up " + inReticle[0].ID;
							actionButtons[0].button.gameObject.SetActive( true );
							robot.selectedObject = inReticle[0].ID;
						}

						if( inReticle.Count > 1 )
						{
							actionButtons[1].text.text = "Pick Up " + inReticle[1].ID;
							actionButtons[1].button.gameObject.SetActive( true );
						}
					}
				}
			}

			for( int i = 0; i < actionButtons.Length && i < lastActionButtonActiveSelf.Length; ++i )
			{
				if( ( actionButtons[i].button.gameObject.activeSelf && !lastActionButtonActiveSelf[i].activeSelf ) || 
				   	( actionButtons[i].text.text != lastActionButtonActiveSelf[i].text ) )
				{
					Ding();

					break;
				}
			}
		}
	}

	protected override void LateUpdate()
	{
		base.LateUpdate();

		for( int i = 0; i < actionButtons.Length && i < lastActionButtonActiveSelf.Length; ++i )
		{
			lastActionButtonActiveSelf[i].activeSelf = actionButtons[i].button.gameObject.activeSelf;
			lastActionButtonActiveSelf[i].text = actionButtons[i].text.text;
		}
	}
}
