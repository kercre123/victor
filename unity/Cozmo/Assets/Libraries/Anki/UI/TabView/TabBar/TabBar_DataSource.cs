using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class TabBar_DataSource : Anki.UI.DataSource {
  public List<string> TabTitles;

  void Start() {
    DataDidUpdate();
  }

  public override uint NumberOfItems(uint section) {
    return (uint)TabTitles.Count;
  }
  
  public override System.Object ObjectAtIndexPath(Anki.UI.IndexPath indexPath) {
    return TabTitles[(int)indexPath.Item];
  }
}
