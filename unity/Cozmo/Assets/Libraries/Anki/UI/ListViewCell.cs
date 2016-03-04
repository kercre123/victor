using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// Base Class for List View Cell. Sub-class this class when making custom Cells for the Anki List view.
/// </summary>

namespace Anki {
  namespace UI {
    [RequireComponent(typeof(LayoutElement))]

    public class ListViewCell : View {
      // Note:
      // - Must setup Layout Element properly to present Cell in the List View
      public LayoutElement LayoutElement { 
        get { return (LayoutElement)gameObject.GetComponent<LayoutElement>(); }
      }


      public bool IsSelected { get { return _IsSelected; } }
      public virtual void SetIsSelected(bool selected) {
        _IsSelected = selected;
      }

      public string ReuseIdentifier { get { return _ReuseIdentifier; } set { _ReuseIdentifier = value; } }
      public Anki.UI.IndexPath IndexPath { get { return _IndexPath; } set { _IndexPath = value; } }

      // Private
      private bool _IsSelected = false;
      private Anki.UI.IndexPath _IndexPath;
      private string _ReuseIdentifier;
    }
  }
}
