using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;

public class TestListView_CellBinding : Anki.UI.ListViewCellBinding {

  public override void OnBindListViewCell(Anki.UI.ListViewCell cell, Anki.UI.IndexPath indexPath) {
    // Cast to cell type
    ListViewDefaultCell theCell = cell as ListViewDefaultCell;
    theCell.Title = (string)_DataSource.ObjectAtIndexPath(indexPath);
    theCell.BackgroundColor = (indexPath.Item % 2 == 0) ? Color.red : Color.blue;
   
    // Play with Cell size 
    cell.LayoutElement.preferredHeight = (indexPath.Item % 2 == 0) ? 160f : 240f;

    theCell.CellButton.onClick.RemoveAllListeners();

    theCell.CellButton.onClick.AddListener(() => {
//      Debug.Log("1 - ON Click");
      if (_ListView.SelectedIndexPath().HasValue) {
//        Debug.Log("2 - Has Value: " + _ListView.SelectedIndexPath().ToString());
        if (_ListView.SelectedIndexPath().Value == indexPath) {
//          Debug.Log("3 - Is already Seleced - Deselect");
          _ListView.DeselectItemAtIndexPath(_ListView.SelectedIndexPath().Value);
        }
        else {
//          Debug.Log("3 - New Value - SelectItem: " + indexPath.ToString());
          _ListView.SelectItemAtIndexPath(indexPath);
        }
      }
      else {
//        Debug.Log("2 - No Value - SelectItem: " + indexPath.ToString());
        _ListView.SelectItemAtIndexPath(indexPath);
      }

    });
  }

  public override void OnUnbindListViewCell(Anki.UI.ListViewCell cell, Anki.UI.IndexPath indexPath) {
    ListViewDefaultCell defaultCell = cell as ListViewDefaultCell;
    defaultCell.CellButton.onClick.RemoveAllListeners();
  }
}
