using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// same as selectObject style, but with an additional button per cube that is linked with a line and placed on the right
///   for easier reach with right thumb
/// </summary>
public class CozmoVision_SelectObjectExtraButtons : CozmoVision_SelectObject {
  protected ObservedObjectButton1[] observedObjectButtons;
  protected List<ObservedObjectBox> unassignedActiveObjectBoxes;

  protected override void Awake() { 
    base.Awake();
    
    unassignedActiveObjectBoxes = new List<ObservedObjectBox>();
    
    observedObjectButtons = observedObjectCanvas.GetComponentsInChildren<ObservedObjectButton1>(true);
    
    for (int i = 0; i < observedObjectBoxes.Count; ++i) {
      ObservedObjectButton1 button = observedObjectBoxes[i] as ObservedObjectButton1;
      
      if (button != null) {
        observedObjectBoxes.RemoveAt(i--);
      }
    }
  }

  protected override void Update() {
    base.Update();
    
    if (robot == null)
      return;
    
    ConnectButtonsToBoxes();
  }

  protected void ConnectButtonsToBoxes() {
    if (IsSmallScreen)
      return;

    unassignedActiveObjectBoxes.Clear();
    unassignedActiveObjectBoxes.AddRange(observedObjectBoxes.FindAll(x => x.gameObject.activeSelf));
    int count = unassignedActiveObjectBoxes.Count;
    
    for (int i = 0; i < observedObjectButtons.Length; ++i) {
      ObservedObjectButton1 button = observedObjectButtons[i];
      button.gameObject.SetActive(i < count && robot.selectedObjects.Count == 0 && !robot.isBusy);
      
      if (!button.gameObject.activeSelf) {
        button.line.points2.Clear();
        button.line.Draw();
        continue;
      }
      
      ObservedObjectBox1 box = null;
      Vector2 buttonPosition = (Vector2)button.position;
      Vector2 boxPosition = Vector2.zero;
      float closest = float.MaxValue;
      
      for (int j = 0; j < unassignedActiveObjectBoxes.Count; ++j) {
        ObservedObjectBox1 box1 = unassignedActiveObjectBoxes[j] as ObservedObjectBox1;
        Vector2 box1Position = box1.position;
        float dist = (buttonPosition - box1Position).sqrMagnitude;
        
        if (dist < closest) {
          closest = dist;
          box = box1;
          boxPosition = box1Position;
        }
      }
      
      if (box == null) {
        Debug.LogError("box shouldn't be null here!");
        continue;
      }
      
      unassignedActiveObjectBoxes.Remove(box);

      button.observedObject = box.observedObject;
      button.box = box;
      button.SetText(box.text.text);
      button.SetTextColor(box.text.color);

      //dmd hide lines when info panels are open
      if (GameLayoutTracker.instance == null || GameLayoutTracker.instance.Phase != LayoutTrackerPhase.DISABLED) {
        buttonPosition = RectTransformUtility.WorldToScreenPoint(button.canvas.renderMode == RenderMode.ScreenSpaceOverlay ? null : canvas.worldCamera, buttonPosition);
        buttonPosition.x += 12f;

        boxPosition = RectTransformUtility.WorldToScreenPoint(box.canvas.renderMode == RenderMode.ScreenSpaceOverlay ? null : canvas.worldCamera, boxPosition);

        button.line.points2.Clear();
        button.line.points2.Add(buttonPosition);
        button.line.points2.Add(boxPosition);
        button.line.SetColor(box.color);
        button.line.Draw();
      }
      else {
        button.line.points2.Clear();
        button.line.Draw();
        box.gameObject.SetActive(false);
      }
    }
  }

  protected override void OnDisable() {
    base.OnDisable();
    
    HideButtonsAndLines();
  }

  void HideButtonsAndLines() {
    for (int i = 0; i < observedObjectButtons.Length; ++i) { 
      if (observedObjectButtons[i] != null) {
        observedObjectButtons[i].gameObject.SetActive(false);
        
        if (observedObjectButtons[i].line != null) {
          observedObjectButtons[i].line.points2.Clear();
          observedObjectButtons[i].line.Draw();
        }
      }
    }
  }
  
}
