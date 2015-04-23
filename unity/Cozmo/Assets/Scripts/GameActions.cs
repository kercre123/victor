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
			if( buttons.Length > 1 )
			{
				if( robot.selectedObjects.Count > 0 )
				{
					buttons[1].SetMode( ActionButtonMode.STACK );
				}
				else if(robot.carryingObject >= 0 && robot.carryingObject.Family == 3)
				{
					buttons[1].SetMode( ActionButtonMode.CHANGE );
				}
			}
			
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
		Debug.Log( "PickUp" );

		if( robot != null ) {
			robot.PickAndPlaceObject( selectedObjectIndex );
			if(CozmoBusyPanel.instance != null)	{

				ObservedObject obj = robot.selectedObjects[selectedObjectIndex];
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
	
	public virtual void Drop()
	{
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
	
	public virtual void Stack()
	{
		Debug.Log( "Stack" );

		if( robot != null ) {
			robot.PickAndPlaceObject( selectedObjectIndex );
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

					ObservedObject target = robot.selectedObjects[selectedObjectIndex];
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

		if( robot != null && robot.carryingObject != null && robot.carryingObject.Family == 3)
		{
			int typeIndex = (int)robot.carryingObject.activeBlockType + 1;
			if(typeIndex >= (int)ActiveBlockType.NumTypes) typeIndex = 0;
			robot.carryingObject.activeBlockType = (ActiveBlockType)typeIndex;
			robot.carryingObject.SetActiveObjectLEDs( CozmoPalette.instance.GetUIntColorForActiveBlockType( robot.carryingObject.activeBlockType ) );
		}
	}
	
	public virtual void Cancel()
	{
		Debug.Log( "Cancel" );
		
		if( robot != null  )
		{
			robot.selectedObjects.Clear();
			robot.SetHeadAngle();
		}
	}
}
