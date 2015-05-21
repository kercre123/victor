using UnityEngine;
using System.Collections;

public class GameActions : MonoBehaviour
{
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected AudioClip[] actionEnabledSounds = new AudioClip[(int)ActionButton.Mode.Count];
	[SerializeField] protected AudioClip[] activeBlockModeSounds = new AudioClip[(int)ActiveBlock.Mode.Count];
	[SerializeField] protected Sprite[] actionSprites = new Sprite[(int)ActionButton.Mode.Count];
	
	public virtual string TARGET { get { if( robot != null && robot.targetLockedObject != null && robot.searching ) return "Object " + robot.targetLockedObject; return "Search"; } }
	public virtual string PICK_UP { get { return "Pick Up"; } }
	public virtual string DROP { get { return "Drop"; } }
	public virtual string STACK { get { return "Stack"; } }
	public virtual string ROLL { get { return "Roll"; } }
	public virtual string ALIGN { get { return "Align"; } }
	public virtual string CHANGE { get { return "Change"; } }
	public virtual string CANCEL { get { return "Cancel Action"; } }

	protected const string TOP = " Top";
	protected const string BOTTOM = " Bottom";

	protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }
	protected ActionButton[] buttons { get { return ActionPanel.instance != null ? ActionPanel.instance.actionButtons : new ActionButton[0]; } }

	public static GameActions instance = null;

	protected virtual void Awake()
	{

	}

	protected virtual void OnEnable()
	{
		//Debug.Log(gameObject.name + " GameActions OnEnable instance = this;");
		instance = this;
	}

	public virtual void OnDisable()
	{
		if(instance == this) {
			//Debug.Log(gameObject.name + " GameActions OnDisable instance = null;");
			instance = null;
		}
	}

	public Sprite GetActionSprite( ActionButton.Mode mode )
	{
		return actionSprites[(int)mode];
	}

	public AudioClip GetActionEnabledSound( ActionButton.Mode mode )
	{
		return actionEnabledSounds[(int)mode];
	}

	public AudioClip GetActiveBlockModeSound( ActiveBlock.Mode mode )
	{
		return activeBlockModeSounds[(int)mode];
	}

	private void CheckChangedButtons()
	{
		for( int i = 0; i < buttons.Length; ++i )
		{
			if( buttons[i].changed ) OnModeEnabled( buttons[i] );
		}
	}

	public void SetActionButtons( bool isSlider = false )
	{
		if( ActionPanel.instance == null ) return;
		
		ActionPanel.instance.SetLastButtons();
		ActionPanel.instance.DisableButtons();

		if( robot == null ) return;

		_SetActionButtons( isSlider );

		CheckChangedButtons();
	}

	protected virtual void _SetActionButtons( bool isSlider ) // 0 is bottom button, 1 is top button, 2 is center button
	{
		if( robot.isBusy ) {
			buttons[2].SetMode( ActionButton.Mode.CANCEL, null, null, true );
			return;
		}
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
//			if( buttons.Length > 1 )
//			{
//				if( robot.carryingObject != null && robot.carryingObject.isActive )
//				{
//					buttons[1].SetMode( ActionButton.Mode.CHANGE, robot.carryingObject );
//				}
//			}

			if( buttons.Length > 1 && robot.selectedObjects.Count > 0 && robot.selectedObjects[0].canBeStackedOn )
			{
				buttons[1].SetMode( ActionButton.Mode.STACK, robot.selectedObjects[0] );
			}

			if( buttons.Length > 0 ) buttons[0].SetMode( ActionButton.Mode.DROP, null );

		}
		else
		{
			if( buttons.Length > 2 && robot.selectedObjects.Count == 1 )
			{
				buttons[1].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[0] );
				buttons[0].SetMode( ActionButton.Mode.ROLL, robot.selectedObjects[0] );
			}
			else
			{
				for( int i = 0; i < robot.selectedObjects.Count && i < 2 && i < buttons.Length; ++i )
				{
					buttons[i].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[i], i == 0 ? BOTTOM : TOP );
				}
			}
		}

		if( buttons.Length > 2 )
		{
			if( isSlider )
			{
				buttons[2].SetMode( ActionButton.Mode.TARGET, null, null, true );
			}
			else if( robot.selectedObjects.Count > 0 )
			{
				buttons[2].SetMode( ActionButton.Mode.CANCEL, null );
			}
		}
	}

	public virtual void OnModeEnabled( ActionButton button )
	{
		AudioClip onEnabledSound = GetActionEnabledSound( button.mode );
		
		if( onEnabledSound != null ) AudioManager.PlayAudioClip( onEnabledSound );
	}

	public virtual void ActionButtonClick()
	{
		AudioManager.PlayOneShot( actionButtonSound );
	}
	
	public virtual void CancelButtonClick()
	{
		AudioManager.PlayOneShot( cancelButtonSound );
	}

	public virtual void PickUp( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease || robot == null ) return;

		ActionButtonClick();

		Debug.Log( "PickUp" );

		robot.PickAndPlaceObject( selectedObject );

		if( CozmoBusyPanel.instance != null ) 
		{
			string desc = null;
			Description( "pick-up\n", selectedObject, ref desc );
			CozmoBusyPanel.instance.SetDescription( desc );
		}
	}
	
	public virtual void Drop( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease || robot == null ) return;

		ActionButtonClick();

		Debug.Log( "Drop" );

		robot.PlaceObjectOnGroundHere();

		if( CozmoBusyPanel.instance != null ) 
		{
			string desc = null;
			Description( "drop\n", selectedObject, ref desc );
			CozmoBusyPanel.instance.SetDescription( desc );
		}
	}
	
	public virtual void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease || robot == null ) return;

		ActionButtonClick();

		Debug.Log( "Stack" );

		robot.PickAndPlaceObject( selectedObject );

		if( CozmoBusyPanel.instance != null ) 
		{
			string desc = null;
			Description( "stack\n", robot.carryingObject, ref desc, string.Empty );
			Description( "\n on top of ", selectedObject, ref desc );
			CozmoBusyPanel.instance.SetDescription( desc );
		}
	}
	
	public virtual void Roll( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease || robot == null ) return;

		ActionButtonClick();

		Debug.Log( "Roll" );

		robot.RollObject( selectedObject );

		if( CozmoBusyPanel.instance != null ) 
		{
			string desc = null;
			Description( "roll\n", selectedObject, ref desc );
			CozmoBusyPanel.instance.SetDescription( desc );
		}
	}
	
	public virtual void Align( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease || robot == null ) return;

		ActionButtonClick();

		Debug.Log( "Align" );
	}
	
	public virtual void Change( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "Change" );

		if( selectedObject != null && selectedObject.isActive )
		{
			ActiveBlock activeBlock = selectedObject as ActiveBlock;

			int typeIndex = (int)activeBlock.mode + 1;
			if(typeIndex >= (int)ActiveBlock.Mode.Count) typeIndex = 0;

			Debug.Log("Changed active block id("+activeBlock+") from " + activeBlock + " to " + (ActiveBlock.Mode)typeIndex );

			activeBlock.mode = (ActiveBlock.Mode)typeIndex;
			activeBlock.SetLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType( activeBlock.mode ) );
			AudioManager.PlayAudioClip( GetActiveBlockModeSound( activeBlock.mode ), actionButtonSound != null ? actionButtonSound.length: 0f );
		}
	}

	public virtual void Cancel( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease || robot == null ) return;

		CancelButtonClick();

		Debug.Log( "Cancel" );
		
		robot.CancelAction();
	}

	public virtual void Target( bool onRelease, ObservedObject selectedObject )
	{
		if( onRelease || robot == null ) return;

		robot.searching = true;
		//Debug.Log( "On Press" );
	}

	private void Description( string verb, ObservedObject selectedObject, ref string description, string period = "." )
	{
		if( selectedObject == null || CozmoPalette.instance == null ) return;

		if( description == null || description == string.Empty ) description = "Cozmo is attempting to ";

		description += verb;
				
		if( selectedObject.isActive )
		{
			description += "an Active Block" + period;
		}
		else
		{
			description += "a " + CozmoPalette.instance.GetNameForObjectType( (int)selectedObject.ObjectType ) + " Block" + period;
		}
	}
}
