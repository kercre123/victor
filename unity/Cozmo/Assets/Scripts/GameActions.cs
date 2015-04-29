using UnityEngine;
using System.Collections;

public class GameActions : MonoBehaviour
{
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected Sprite[] actionSprites = new Sprite[(int)ActionButton.Mode.NUM_MODES];
	
	public virtual string TARGET { get { if( robot != null && robot.targetLockedObject != null && robot.searching ) return "Object " + robot.targetLockedObject; return "Search"; } }
	public virtual string PICK_UP { get { return "Pick Up"; } }
	public virtual string DROP { get { return "Drop"; } }
	public virtual string STACK { get { return "Stack"; } }
	public virtual string ROLL { get { return "Roll"; } }
	public virtual string ALIGN { get { return "Align"; } }
	public virtual string CHANGE { get { return "Change"; } }
	public virtual string CANCEL { get { return "Cancel"; } }

	protected virtual string TOP { get { return " TOP"; } }
	protected virtual string BOTTOM { get { return " BOTTOM"; } }

	protected Robot robot;
	protected ActionButton[] buttons;

	public static GameActions instance = null;

	protected virtual void OnEnable()
	{
		instance = this;
	}

	public virtual void OnDisable()
	{
		if( instance == this ) instance = null;
	}

	public Sprite GetActionSprite( ActionButton.Mode mode )
	{
		return actionSprites[(int)mode];
	}

	public virtual void SetActionButtons( bool isSlider = false ) // 0 is bottom button, 1 is top button, 2 is center button
	{
		if( ActionPanel.instance == null ) return;

		ActionPanel.instance.DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;
		buttons = ActionPanel.instance.actionButtons;

		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( buttons.Length > 1 )
			{
				if(robot.carryingObject >= 0 && robot.carryingObject.Family == 3)
				{
					buttons[1].SetMode( ActionButton.Mode.CHANGE, robot.carryingObject );
				}

			}

			bool stack = false;

			if( robot.selectedObjects.Count > 0 )
			{
				float distance = ((Vector2)robot.selectedObjects[0].WorldPosition - (Vector2)robot.WorldPosition).magnitude;
				if(distance <= CozmoUtil.BLOCK_LENGTH_MM * 2f) {
					buttons[0].SetMode( ActionButton.Mode.STACK, robot.selectedObjects[0] );
					stack = true;
				}
			}

			if(!stack) buttons[0].SetMode( ActionButton.Mode.DROP, null );
		}
		else
		{
			if( robot.selectedObjects.Count == 1 )
			{
				buttons[1].SetMode( ActionButton.Mode.PICK_UP, robot.selectedObjects[0] );
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
				buttons[2].SetMode( ActionButton.Mode.TARGET, null );
			}
			else if( robot.selectedObjects.Count > 0 )
			{
				buttons[2].SetMode( ActionButton.Mode.CANCEL, null );
			}
		}
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

	public virtual void PickUp( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "PickUp" );

		if( robot != null ) {
			robot.PickAndPlaceObject( selectedObject );
			if(CozmoBusyPanel.instance != null)	{

				ObservedObject obj = selectedObject;
				if(obj != null) {
					string desc = "Cozmo is attempting to pick-up\n";

					if(obj.Family == 3) {
						desc += "an Active Block.";
					}
					else {
						desc += "a ";

						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)obj.ObjectType) + " ";
						}

						desc += "Block.";
					}


					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
		}
	}
	
	public virtual void Drop( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "Drop" );

		if( robot != null ) {
			robot.PlaceObjectOnGroundHere();
			if(CozmoBusyPanel.instance != null)	{
				
				ObservedObject obj = robot.carryingObject;
				if(obj != null) {
					string desc = "Cozmo is attempting to drop\n";
					
					if(obj.Family == 3) {
						desc += "an Active Block.";
					}
					else {
						desc += "a ";
						
						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)obj.ObjectType) + " ";
						}
						
						desc += "Block.";
					}
					
					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
		}
	}
	
	public virtual void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "Stack" );

		if( robot != null ) {
			robot.PickAndPlaceObject( selectedObject );
			if(CozmoBusyPanel.instance != null)	{
				
				ObservedObject obj = robot.carryingObject;
				if(obj != null) {
					string desc = "Cozmo is attempting to stack\n";
					
					if(obj.Family == 3) {
						desc += "an Active Block";
					}
					else {
						desc += "a ";
						
						if(CozmoPalette.instance != null) {
							desc += CozmoPalette.instance.GetNameForObjectType((int)obj.ObjectType) + " ";
						}
						
						desc += "Block";
					}

					ObservedObject target = selectedObject;
					if(target != null) {
						
						desc += "\n on top of ";

						if(target.Family == 3) {
							desc += "an Active Block";
						}
						else {
							desc += "a ";
							
							if(CozmoPalette.instance != null) {
								desc += CozmoPalette.instance.GetNameForObjectType((int)target.ObjectType) + " ";
							}
							
							desc += "Block";
						}
					}

					desc += ".";

					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
		}
	}
	
	public virtual void Roll( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "Roll" );
	}
	
	public virtual void Align( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "Align" );
	}
	
	public virtual void Change( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		ActionButtonClick();

		Debug.Log( "Change" );

		if( robot != null && robot.carryingObject != null && robot.carryingObject.Family == 3)
		{
			int typeIndex = (int)robot.carryingObject.activeBlockType + 1;
			if(typeIndex >= (int)ActiveBlockType.NumTypes) typeIndex = 0;
			robot.carryingObject.activeBlockType = (ActiveBlockType)typeIndex;
			robot.carryingObject.SetActiveObjectLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType( robot.carryingObject.activeBlockType ) );
		}
	}
	
	public virtual void Cancel( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		CancelButtonClick();

		Debug.Log( "Cancel" );
		
		if( robot != null )
		{
			robot.selectedObjects.Clear();
			robot.SetHeadAngle();
			robot.targetLockedObject = null;
		}
	}

	public virtual void Target( bool onRelease, ObservedObject selectedObject )
	{
		if( onRelease )
		{
			robot.searching = false;
		}
		else
		{
			robot.searching = true;
		}
	}
}
