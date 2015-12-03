using UnityEngine;
using System.Runtime.InteropServices;
using System.Collections;

// NOTE: Not handling Section yet. Everything must be in section 0

namespace Anki {
  namespace UI {
    [RequireComponent(typeof(ListView))]
    [RequireComponent(typeof(DataSource))]

    public class ListViewCellBinding : Anki.BaseBehaviour {
      // Default Cell Prefab outlet
      public ListViewCell CellPrefab;

      // Return an Instance of the Cell
      // Note: Needed to brake this out of CellForIndexPath so we can add reusable cells
      public virtual Anki.UI.ListViewCell CreateCellInstanceForIndexPath(Anki.UI.IndexPath indexPath) {
        ListViewCell cell = null;
        if (CellPrefab != null) {
          cell = Instantiate(CellPrefab) as ListViewCell;
        }
        return cell;
      }

      public virtual Anki.UI.ListViewCell CellForIndexPath(Anki.UI.IndexPath indexPath) {
        string reuseId = CellIdentifierForIndexPath(indexPath);
        ListViewCell cell = _ListView.DequeueReusableCell(reuseId, indexPath);
        if (cell == null) {
          cell = CreateCellInstanceForIndexPath(indexPath);
          cell.ReuseIdentifier = reuseId;
          cell.IndexPath = indexPath;
        }
        return cell;
      }

      public virtual string CellIdentifierForIndexPath(Anki.UI.IndexPath indexPath) {
        return "default";
      }

      // Bind Cell With Data
      public virtual void OnBindListViewCell(Anki.UI.ListViewCell cell, Anki.UI.IndexPath indexPath) {
        // Subclasses need to override in order to properly update the view and register for events.
      }

      // Unbind Cell
      public virtual void OnUnbindListViewCell(Anki.UI.ListViewCell cell, Anki.UI.IndexPath indexPath) {
        // Subclasses need to override in order to properly unregister for events.
      }

      // Access other compnents needed in ListViewCellBinding class
      protected Anki.UI.ListView _ListView {
        get { return GetComponent<ListView>(); }
      }

      protected Anki.UI.DataSource _DataSource {
        get { return _ListView.DataSource; }
      }
    }
  }
}
