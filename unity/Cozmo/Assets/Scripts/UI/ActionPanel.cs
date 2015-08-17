using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// wrapper for our action buttons
///   managed by the CozmoVision system
///     which uses the GameActions to control which buttons are visible,
///     how they can be interacted with,
///     and which actions they request
/// </summary>
public class ActionPanel : MonoBehaviour
{
  public ActionButton[] actionButtons;
  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }
  protected int selectedObjectIndex;

  public bool IsSmallScreen { get; protected set; }

  public bool secondaryActionsAvailable
  {
    get
    {
      for( int i = 0, count = 0; i < actionButtons.Length; ++i )
      {
        if( actionButtons[i].mode != ActionButton.Mode.DISABLED && ++count > 1 ) return true;
      }

      return false;
    }
  }

  public bool nonSearchActionAvailable
  {
    get
    {
      for( int i = 0; i < actionButtons.Length; ++i )
      {
        if( actionButtons[i].mode != ActionButton.Mode.DISABLED && actionButtons[i].mode != ActionButton.Mode.TARGET ) return true;
      }

      return false;
    }
  }

  public bool allDisabled
  {
    get
    {
      for( int i = 0; i < actionButtons.Length; ++i )
      {
        if( actionButtons[i].mode != ActionButton.Mode.DISABLED ) return false;
      }
      
      return true;
    }
  }

  public static ActionPanel instance = null;

  protected virtual void Awake()
  {
  }

  public void DisableButtons() {
    for(int i=0; i<actionButtons.Length; i++) actionButtons[i].SetMode(ActionButton.Mode.DISABLED, null);
  }

  public void SetLastButtons() {
    for(int i=0; i<actionButtons.Length; i++) actionButtons[i].SetLastMode();
  }

  protected virtual void OnEnable()
  {
    instance = this;
  }

  protected virtual void OnDisable()
  {
    if(instance == this) instance = null;
  }
}
