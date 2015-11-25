// DEPRECATED (DO NOT USE THIS!!) use ListView instead

using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

namespace AnkiList {
  [RequireComponent(typeof(ScrollRect))]

  public class AnkiListView : Anki.BaseBehaviour {
    private AnkiListViewDataSource m_dataSource;

    public AnkiListViewDataSource dataSource {
      get { return m_dataSource; }
      set { m_dataSource = value; }
    }

    public void ReloadData() {
      DestroyListCells();

      if (m_scrollRect != null && m_dataSource != null) {
        m_numberOfRows = m_dataSource.GetNumberOfRowsForListView(this);
        LayoutListCells();
      }
    }

    [SerializeField]
    private ScrollRect
      m_scrollRect;
    private int m_numberOfRows;
    private List<AnkiListViewCell> m_cachedCells = new List<AnkiListViewCell>();

    protected override void Awake() {
      base.Awake();
      m_scrollRect = GetComponent<ScrollRect>();
    }

    protected override void OnEnable() {
      base.OnEnable();
      ReloadData();
    }
  
    // responsible for laying out cells in the list container
    private void LayoutListCells() {
      // Create & Layout Cells
      for (int idx = 0; idx < m_numberOfRows; ++idx) {

        AnkiListViewCell cell = m_dataSource.GetCellForRowInListView(this, idx);

        cell.transform.SetParent(m_scrollRect.content, false);
        cell.transform.localScale = m_scrollRect.content.localScale;

        LayoutElement layoutElement = cell.GetComponent<LayoutElement>();
        if (layoutElement == null) {
          layoutElement = cell.gameObject.AddComponent<LayoutElement>();
        }

        m_cachedCells.Add(cell);
      }

      LayoutRebuilder.MarkLayoutForRebuild(m_scrollRect.content);

    }

    // responsible for cleaning up cells in the list container
    private void DestroyListCells() {

      if (m_cachedCells == null)
        return;

      for (int k = 0; k < m_cachedCells.Count; ++k) {
        AnkiListViewCell aCell = m_cachedCells[k];
        Destroy(aCell.gameObject);
      }
      m_cachedCells.Clear();
    }
  }

}
