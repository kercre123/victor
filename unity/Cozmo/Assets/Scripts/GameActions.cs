using UnityEngine;
using System.Collections;

public class GameActions : MonoBehaviour
{
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];

	public virtual string TARGET { get { return "Search"; } }
	public virtual string PICK_UP { get { return "Pick Up"; } }
	public virtual string DROP { get { return "Drop"; } }
	public virtual string STACK { get { return "Stack"; } }
	public virtual string ROLL { get { return "Roll"; } }
	public virtual string ALIGN { get { return "Align"; } }
	public virtual string CHANGE { get { return "Change"; } }
	public virtual string CANCEL { get { return "Cancel"; } }

	protected virtual string TOP { get { return " TOP"; } }
	protected virtual string BOTTOM { get { return " BOTTOM"; } }

	[System.NonSerialized] public int selectedObjectIndex;

	protected Robot robot;
	protected ActionButton[] buttons;

	protected virtual IEnumerator SetActionPanelReference()
	{
		while( ActionPanel.instance == null )
		{
			yield return null;
		}

		if( ActionPanel.instance.gameActions != this )
		{
			ActionPanel.instance.gameActions = this;
		}
	}

	protected virtual void OnEnable()
	{
		StartCoroutine( SetActionPanelReference() );
	}

	public virtual void OnDisable()
	{
		if( ActionPanel.instance != null && ActionPanel.instance.gameActions == this ) ActionPanel.instance.gameActions = null;
	}

	public Sprite GetActionSprite( ActionButtonMode mode )
	{
		return actionSprites[(int)mode];
	}

	public virtual void SetActionButtons() // 0 is bottom button, 1 is top button
	{
		if( ActionPanel.instance == null ) return;

		ActionPanel.instance.DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;
		buttons = ActionPanel.instance.actionButtons;
		
		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( robot.selectedObjects.Count > 0 && buttons.Length > 1 ) buttons[1].SetMode( ActionButtonMode.STACK );
			
			buttons[0].SetMode( ActionButtonMode.DROP );
		}
		else
		{
			if( robot.selectedObjects.Count == 1 )
			{
				buttons[1].SetMode( ActionButtonMode.PICK_UP );
			}
			else
			{
				for( int i = 0; i < robot.selectedObjects.Count && i < 2 && i < buttons.Length; ++i )
				{
					buttons[i].SetMode( ActionButtonMode.PICK_UP, i, i == 0 ? BOTTOM : TOP );
				}
			}
		}
		
		if( robot.selectedObjects.Count > 0 && buttons.Length > 2 ) buttons[2].SetMode( ActionButtonMode.CANCEL );
	}


	public virtual void ActionButtonClick()
	{
		if( audio != null )
		{
			audio.volume = 1f;
			audio.PlayOneShot( actionButtonSound, 1f );
		}
	}
	
	public virtual void CancelButtonClick()
	{
		if( audio != null )
		{
			audio.volume = 1f;
			audio.PlayOneShot( cancelButtonSound, 1f );
		}
	}

	public virtual void PickUp()
	{
		if( RobotEngineManager.instance != null ) RobotEngineManager.instance.current.PickAndPlaceObject( selectedObjectIndex );
	}
	
	public virtual void Drop()
	{
		if( RobotEngineManager.instance != null ) RobotEngineManager.instance.current.PlaceObjectOnGroundHere();
	}
	
	public virtual void Stack()
	{
		if( RobotEngineManager.instance != null ) RobotEngineManager.instance.current.PickAndPlaceObject( selectedObjectIndex );
	}
	
	public virtual void Roll()
	{
		Debug.Log( "Roll" );
	}
	
	public virtual void Align()
	{
		Debug.Log( "Align" );
	}
	
	public virtual void Change()
	{
		Debug.Log( "Change" );
	}
	
	public virtual void Cancel()
	{
		Debug.Log( "Cancel" );
		
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.selectedObjects.Clear();
			RobotEngineManager.instance.current.SetHeadAngle();
		}
	}
}
