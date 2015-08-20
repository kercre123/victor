using UnityEngine;
using System;
using System.Collections;


/// <summary>
/// for the cozmo prototype thus far, we are merely concerned with whether the device is an iPad (4/3) or iPhone(16/9)
///   this logic may need to get more robust as more devices are supported
/// </summary>
[ExecuteInEditMode]
public class CozmoScreenMultiSettingsDetector : ScreenMultiSettingsDetector {
  protected override void UpdateIndex() {

    Vector2 screenSize = new Vector2(Screen.width, Screen.height);

    
    float aspect = 1f;
    if (screenSize.y > 0f)
      aspect = screenSize.x / screenSize.y;
    
    bool wideScreen = aspect > (16f / 10.5f);
    

    CurrentIndex = wideScreen ? 1 : 0;
  }

}
