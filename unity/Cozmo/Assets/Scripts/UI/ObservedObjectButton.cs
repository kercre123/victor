using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectButton : ObservedObjectBox {
  [SerializeField] protected AudioClip select;

  Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public virtual void Selection() { 
    if (robot != null) {
      robot.selectedObjects.Clear();
      robot.selectedObjects.Add(observedObject);
      robot.TrackToObject(observedObject);
    }

    AudioManager.PlayOneShot(select);
  }
}
