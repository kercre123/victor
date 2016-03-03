using UnityEngine;
using System.Collections;

namespace Anki {
  namespace UI {
    [RequireComponent(typeof(TabView))]
    [RequireComponent(typeof(DataSource))]

    public class TabViewCellBinding : Anki.BaseBehaviour {
      // Default Cell Prefab outlet
      public TabViewCell CellPrefab;

      // Return an Instance of the Cell
      // Note: Needed to brake this out of CellForIndexPath so we can add reusable cells
      public virtual TabViewCell CreateCellInstanceForIndexPath(Anki.UI.IndexPath indexPath) {
        TabViewCell cell = null;
        if (CellPrefab != null) {
          cell = Instantiate(CellPrefab) as TabViewCell;
        }
        return cell;
      }
      
      public virtual TabViewCell CellForIndexPath(Anki.UI.IndexPath indexPath) {
        // Do we need to reuse some stuff?
        TabViewCell cell = CreateCellInstanceForIndexPath(indexPath);
        cell.IndexPath = indexPath;
        return cell;
      }

      // Bind Cell With Data
      public virtual void OnBindTabViewCell(Anki.UI.TabViewCell cell, Anki.UI.IndexPath indexPath) {
        // Subclasses need to override in order to properly update the view and register for events.
      }

      // Unbind Cell
      public virtual void OnUnbindTabViewCell(Anki.UI.TabViewCell cell, Anki.UI.IndexPath indexPath) {
        // Subclasses need to override in order to properly unregister for events.
      }

      // Access other compnents needed in AnkiTabViewCellBinding class
      protected Anki.UI.TabView _TabView {
        get { return GetComponent<TabView>(); }
      }
      
      protected Anki.UI.DataSource _DataSource {
        get { return _TabView.DataSource; }
      }
    }
  }
}
