using UnityEngine;
using System.Collections;
using Anki.Core.UI.Components;
// Skeleton class for incase we want to add additional custom functionality to our UI images 
public class CozmoImage : AnkiImage {
  public void UpdateSkinnableElements() {
    UpdateSkinnableElements(CozmoThemeSystemUtils.sInstance.GetCurrentThemeId(), CozmoThemeSystemUtils.sInstance.GetCurrentSkinId());
  }
}
