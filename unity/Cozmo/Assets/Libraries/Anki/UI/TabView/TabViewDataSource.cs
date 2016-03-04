using UnityEngine;
using System.Collections;
using Anki.UI;
using System.Collections.Generic;

namespace Anki {
  namespace UI {
    public class TabViewDataSource : Anki.UI.DataSource {
      public virtual List<string> GetTabTitles() {
        return null;
      }
    }
  }
}
