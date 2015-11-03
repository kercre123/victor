using UnityEngine;
using System.Collections;

public class DebugMenuTestCanvas : MonoBehaviour {
  
  // TODO: Pragma out for production
  #region TESTING
  public void OnAddInfoTap(){
    DAS.Info("DebugMenuManager", "Manually Adding Info Info Info");
  }
  
  public void OnAddWarningTap(){
    DAS.Warn("DebugMenuManager", "Manually Adding Warn Warn Warn");
  }
  
  public void OnAddErrorTap(){
    DAS.Error("DebugMenuManager", "Manually Adding Error Error Error");
  }
  
  public void OnAddEventTap(){
    DAS.Event("DebugMenuManager", "Manually Adding Event Event Event");
  }
  
  public void OnAddDebugTap(){
    DAS.Debug("DebugMenuManager", "Manually Adding Debug Debug Debug");
  }
  #endregion
}
