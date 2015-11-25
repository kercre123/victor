using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using System;

namespace Anki {
  namespace UI {

    [RequireComponent(typeof(TabViewCellBinding))]
    [RequireComponent(typeof(TabViewDataSource))]

    public class TabView : View {
      public event Action OnTabBarBackButtonClicked;

      // Tab Bar
      public TabBar TabBar;
      public float TabBarHeight = 120.0f;

      // Data Source & Cell Binding
      private TabViewDataSource _DataSource;   
      private TabViewCellBinding _CellBinding;

      private List<TabViewCell> _TabCells = new List<TabViewCell>();
      private uint _NumberOfTabs = 0;
      private uint _CurrentTab = 0;

      private List<Button> _TabButtons;

      public TabViewDataSource DataSource {
        get { return _DataSource; }
        set {
          // Unregister old data source
          if (_DataSource != null) {
            _DataSource.OnObjectDataDidUpdate -= ReceivedOnObjectDataDidUpdate;
            _DataSource.OnDataDidUpdate -= ReceivedOnDataDidUpdate;
          }
          
          // Set 
          _DataSource = value;
          if (_DataSource != null) {
            // Register
            _DataSource.OnObjectDataDidUpdate += ReceivedOnObjectDataDidUpdate;
            _DataSource.OnDataDidUpdate += ReceivedOnDataDidUpdate;
          }
        }
      }

      public TabViewCellBinding CellBinding {
        get { return _CellBinding; }
        set { _CellBinding = value; }
      }

      protected override void OnLoad() {
        // Check if Data Source or Cell Bindings were added as an component
        // Remove from component after setting property
        DataSource = GetComponent<TabViewDataSource>();
        CellBinding = GetComponent<TabViewCellBinding>();

        TabBar = (TabBar)GameObject.Instantiate(TabBar);
      }

      public void ReloadData() {
        // DAS.Debug("ListView", "ReloadData()");
        if (DependantComponentsAreReady()) {

          // Should re-layout the items to determine what's visible
          DestroyTabCells();

          _NumberOfTabs = DataSource.NumberOfItems(0);

          LayoutTabBar();
          LayoutTabCells();
        }
      }

      // Respond to DataSource Action methods
      protected virtual void ReceivedOnObjectDataDidUpdate(List<Anki.UI.IndexPath> updatedIndexPaths) {
        // TODO: Add Support for Sections
      }
      
      protected virtual void ReceivedOnDataDidUpdate() {
        ReloadData();
      }

      // Responsbile for laying out the tab bar
      private void LayoutTabBar() {
        TabBar.gameObject.transform.SetParent(this.gameObject.transform, false);
        LayoutTabBarTransform(TabBar.gameObject);

        TabBar.SetTabBarTitles(DataSource.GetTabTitles());
        TabBar.ReloadData();

        UnregisterTabHandlers();
        RegisterTabHandlers();

        UnregisterTabBarBackButtonHandler();
        RegisterTabBarBackButtonHandler();
      }

      // Responsible for laying out cells in the tab list
      private void LayoutTabCells() {
        // Create & Layout Cells
        for (uint i = 0; i < _NumberOfTabs; ++i) {
          IndexPath idxPath = new IndexPath(0, i);
          
          TabViewCell cell = CreateAndAddTabForIndexPath(idxPath);
          DebugUtils.Assert(cell != null);

          _TabCells.Add(cell);
          _CellBinding.OnBindTabViewCell(cell, idxPath);
        }       
        ShowActiveTab();
      }

      // Hide all the nonactive tabs and show current active tab
      private void ShowActiveTab() {
        for (int i = 0; i < (int)_NumberOfTabs; ++i) {
          TabViewCell cell = _TabCells[i];
          if (i == _CurrentTab) {
            cell.gameObject.SetActive(true);
          }
          else {
            cell.gameObject.SetActive(false);
          }
        }
      }

      // Note: Index Path Must be valid
      private TabViewCell CreateAndAddTabForIndexPath(IndexPath indexPath) {
        DebugUtils.Assert(DataSource != null);
        DebugUtils.Assert(DataSource.IsIndexPathValid(indexPath));
        DebugUtils.Assert(CellBinding != null);

        TabViewCell cell = null;
        
        cell = _CellBinding.CellForIndexPath(indexPath);
        cell.IndexPath = indexPath;
        // Set transform and position cell
        if (cell != null) {
          cell.transform.SetParent(this.gameObject.transform, false);
          LayoutTabCellTransform(cell.gameObject);
        }
        
        return cell;
      }

      // Responsible for destroying all the cells
      private void DestroyTabCells() {
        for (int i = 0; i < _TabCells.Count; ++i) {
          TabViewCell cell = _TabCells[i];
          CellBinding.OnUnbindTabViewCell(cell, cell.IndexPath);
        }

        _TabCells.Clear();
      }

      // Setup Navigation Bar Layout
      private void LayoutTabBarTransform(GameObject navigationBarGameObject) { 
        RectTransform rectTransform = navigationBarGameObject.GetComponent<RectTransform>();
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = new Vector2(0.0f, TabBarHeight);
        rectTransform.anchoredPosition = Vector2.zero;
        rectTransform.localScale = Vector3.one;
      }

      // Set up layout for tab
      private void LayoutTabCellTransform(GameObject tabCellGameObject) {
        RectTransform rectTransform = tabCellGameObject.GetComponent<RectTransform>();

        rectTransform.offsetMin = new Vector2(rectTransform.offsetMin.x, rectTransform.offsetMin.y);
        rectTransform.offsetMax = new Vector2(rectTransform.offsetMax.x, TabBarHeight * -1);

        rectTransform.localScale = Vector3.one;
      }

      void HandleOnTabClicked(IndexPath index) {
        DAS.Debug("TabView::HandleOnTabClicked", "Index: " + index.Item);
        _CurrentTab = index.Item;
        ShowActiveTab();
      }

      void HandleOnTabBarBackButtonClicked() {
        if (OnTabBarBackButtonClicked != null) {
          OnTabBarBackButtonClicked();
        }
      }

      private bool _DependantComponentsAreReadyFlag = false;
      private bool DependantComponentsAreReady() {
        if (_DependantComponentsAreReadyFlag == false) {
          if (DataSource != null && CellBinding != null) {
            _DependantComponentsAreReadyFlag = (DataSource.isActiveAndEnabled && CellBinding.isActiveAndEnabled);
          }
        }
        
        return _DependantComponentsAreReadyFlag;
      }

      private void RegisterTabBarBackButtonHandler() {
        if (TabBar != null) {
          TabBar.BackButton.onClick.AddListener(() => {
            HandleOnTabBarBackButtonClicked();
          });
        }
      }

      private void UnregisterTabBarBackButtonHandler() {
        if (TabBar != null) {
          TabBar.BackButton.onClick.RemoveAllListeners();
        }
      }

      private void RegisterTabHandlers() {
        if (TabBar != null && TabBar.TabListView != null && TabBar.TabListView.CellBinding != null) {
          TabBar_CellBinding tabBarBinding = (TabBar_CellBinding)TabBar.TabListView.CellBinding;
          tabBarBinding.OnTabClicked += HandleOnTabClicked;
        }
      }
      
      private void UnregisterTabHandlers() {
        if (TabBar != null && TabBar.TabListView != null && TabBar.TabListView.CellBinding != null) {
          TabBar_CellBinding tabBarBinding = (TabBar_CellBinding)TabBar.TabListView.CellBinding;
          tabBarBinding.OnTabClicked -= HandleOnTabClicked;
        }
      }

    }
  }
}