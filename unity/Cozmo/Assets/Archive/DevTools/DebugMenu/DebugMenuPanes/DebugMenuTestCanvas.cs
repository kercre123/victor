using UnityEngine;
using System.Collections;

public class DebugMenuTestCanvas : MonoBehaviour {
  
  // TODO: Pragma out for production
  #region TESTING
  public void OnAddInfoTap(){
    DAS.Info(this, "Manually Adding Info Info Info");
  }
  
  public void OnAddWarningTap(){
    DAS.Warn(this, "Manually Adding Warn Warn Warn");
  }
  
  public void OnAddErrorTap(){
    DAS.Error(this, "Manually Adding Error Error Error");
  }
  
  public void OnAddEventTap(){
    DAS.Event(this, "Manually Adding Event Event Event");
  }
  
  public void OnAddDebugTap(){
    DAS.Debug(this, "Manually Adding Debug Debug Debug");
  }
  #endregion
}
