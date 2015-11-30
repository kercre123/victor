using UnityEngine;
using System.Collections;
using Anki.UI;
using System;

public class TabBar_CellBinding : Anki.UI.ListViewCellBinding {
  public event Action<Anki.UI.IndexPath> OnTabClicked;

  public override void OnBindListViewCell(ListViewCell cell, IndexPath indexPath) {
    
    TabBar_ItemCell tabBarItemCell = (TabBar_ItemCell)cell; 
    string tabTitle = (string)_DataSource.ObjectAtIndexPath(indexPath);

    tabBarItemCell.TabText.text = tabTitle;

    tabBarItemCell.TabButton.onClick.AddListener(() => {
      DAS.Debug("TabBar_CellBinding:OnBindListViewCell", "OnTabClicked: " + tabTitle);
      HandleTabClick(indexPath);
    });
  }

  public override void OnUnbindListViewCell(ListViewCell cell, IndexPath indexPath) {
    TabBar_ItemCell tabBarItemCell = (TabBar_ItemCell)cell; 
    tabBarItemCell.TabButton.onClick.RemoveAllListeners();
  }

  private void HandleTabClick(IndexPath indexPath) {
    if (OnTabClicked != null) {
      OnTabClicked(indexPath);
    }
  }
}
