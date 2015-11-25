using UnityEngine;
using System.Collections;

namespace AnkiList
{
  public interface AnkiListViewDataSource
  {
    int GetNumberOfRowsForListView(AnkiListView listView);

    AnkiListViewCell GetCellForRowInListView(AnkiListView listView, int row);

  }
}
