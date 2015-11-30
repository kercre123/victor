using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;

namespace Anki {
  namespace UI {
    public class TabBar : Anki.UI.View {
      public AnkiButton BackButton;
      public ListView TabListView;

      public bool BackButtonHidden {
        get {
          if (BackButton == null) {
            return true;
          }
          else {
            return !BackButton.IsActive();
          }
        }
        set {
          if (BackButton != null) {
            BackButton.gameObject.SetActive(!value);
          }
        }
      }

      public void SetTabBarTitles(List<string> titles) {
        TabBar_DataSource tabBarDataSource = (TabBar_DataSource)TabListView.DataSource;
        tabBarDataSource.TabTitles = titles;
      }

      public void ReloadData() {
        TabListView.ReloadData();
      }
    }
  }
}
