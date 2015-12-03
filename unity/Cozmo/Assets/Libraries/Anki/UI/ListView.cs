using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;


// TODO Not handling sections yet

namespace Anki {
  namespace UI {

    [RequireComponent(typeof(ListViewCellBinding))]
    [RequireComponent(typeof(DataSource))]

    public class ListView : View {

      // List Content Aligmnet Enum
      public enum ContentViewAlignment { Beginning, Middle, End };


      public new static View CreateInstance(string name) {
        View instance = new GameObject((name == null) ? "ListView" : name).AddComponent<ListView>();

        RectTransform rectTransform = instance.gameObject.AddComponent<RectTransform>();
        rectTransform.anchorMin = Vector2.zero;
        rectTransform.anchorMax = new Vector2(1.0f, 1.0f);
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
        
        return instance;
      }

      public DataSource DataSource {
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

      public ListViewCellBinding CellBinding {
        get { return _CellBinding; }
        set { _CellBinding = value; }
      }

      public bool IsHorizontalScrollDirection {
        get { return _IsHorizontalScrollDirection; }
        set { 
          // TODO: Need to test this using a script
          if (_IsHorizontalScrollDirection != value) {
            _IsHorizontalScrollDirection = value;
            _LayoutListViewScrollDirection();
            ReloadData();
          }
        }
      }


      public ContentViewAlignment ListAlignment {
        set {
          _ListAlignment = value;
          Vector2? pivot = null;
          switch (_ListAlignment) {
            case ContentViewAlignment.Beginning:
            {
              if (_IsHorizontalScrollDirection) {
                pivot = new Vector2(0.0f, 0.5f);
              }
              else {
                pivot = new Vector2(0.5f, 1.0f);
              }
            }
              break;

            case ContentViewAlignment.Middle:
            {
                pivot = new Vector2(0.5f, 0.5f);
            }
              break;

            case ContentViewAlignment.End:
            {
              if (_IsHorizontalScrollDirection) {
                pivot = new Vector2(1.0f, 0.5f);
              }
              else {
                pivot = new Vector2(0.5f, 0.0f);
              }
            }
              break;
          }
          if (pivot.HasValue) {
            RectTransform rectTransform = _ScrollRectContentObject.GetComponent<RectTransform>();
            rectTransform.pivot = pivot.Value;
          }
        }
        get { return _ListAlignment; }
      }

      public IndexPath? SelectedIndexPath() {
        return _SelectedIndexPath;
      }

      private bool _IsCellVisible(IndexPath indexPath) {
        return (-1 != _VisibleCells.FindIndex(delegate (ListViewCell cell) {
          return cell.IndexPath == indexPath;
        }));
      }

      public ListViewCell VisibleCellAtIndexPath(IndexPath indexPath) {
        return _VisibleCellAtIndexPath(indexPath);
      }

      private ListViewCell _VisibleCellAtIndexPath(IndexPath indexPath) {
        // This is O(N), which sucks.  We could do better if we knew the list was sorted.
        ListViewCell result = null;
        result = _VisibleCells.Find(delegate (ListViewCell cell) {
          return cell.IndexPath == indexPath;
        });
        return result;
      }

      public void SelectItemAtIndexPath(IndexPath indexPath) {
        // TODO: Add Support for Sections

        IndexPath? previousSelectedPath = _SelectedIndexPath;
        _SelectedIndexPath = indexPath;
        // Clear Current Cell

        if (previousSelectedPath.HasValue && _IsCellVisible(previousSelectedPath.Value)) {
          ListViewCell cell = null;
          cell = _VisibleCellAtIndexPath(previousSelectedPath.Value);
          UpdateCellAtIndexPath(cell, previousSelectedPath.Value);
        }
        // Set New Cell
        if (_SelectedIndexPath.HasValue && _IsCellVisible(_SelectedIndexPath.Value)) {
          ListViewCell cell = null;
          cell = _VisibleCellAtIndexPath(_SelectedIndexPath.Value);
          UpdateCellAtIndexPath(cell, _SelectedIndexPath.Value);
        }
      }

      public void DeselectItemAtIndexPath(IndexPath indexPath) {
        DAS.Debug("ListView", "DeselectItemAtIndexPath: " + indexPath.ToString());
        // TODO: Add Support for Sections
        // Clear Selected Cell
        if (_SelectedIndexPath.HasValue && _SelectedIndexPath.Value == indexPath) {
          _SelectedIndexPath = null;
          if (_IsCellVisible(indexPath)) {
            ListViewCell cell = _VisibleCellAtIndexPath(indexPath);
            UpdateCellAtIndexPath(cell, indexPath);
          }
        }
      }

      public void ReloadData() {
        // DAS.Debug("ListView", "ReloadData()");
        if (_DependantComponentsAreReady()) {

          if (!_DidLayoutScrollDirection) {
            _LayoutListViewScrollDirection();
          }
          // Should re-layout the items to determine what's visible
          _DestroyListCells();
		  
          _NumberOfRows = DataSource.NumberOfItems(0);
          _LayoutListCells();
        }
      }

      public void ReloadDataAtIndexPaths(List<Anki.UI.IndexPath> indexPaths) {
        if (_DependantComponentsAreReady()) {

          if (!_DidLayoutScrollDirection) {
            _LayoutListViewScrollDirection();
          }

          for (int i = 0; i < indexPaths.Count; ++i) {
            Anki.UI.IndexPath ip = indexPaths[i];
            ListViewCell cell = _VisibleCellAtIndexPath(ip);
            if (null != cell) {
              UpdateCellAtIndexPath(cell, ip);
            }
          }
        }
      }

      // Cell pooling
      public ListViewCell DequeueReusableCell(string identifier) {
        ListViewCell result = null;
        if (_UnusedCells.ContainsKey(identifier)) {
          List<ListViewCell> unusedCells = _UnusedCells[identifier];
          if (unusedCells.Count > 0) {
            result = unusedCells[unusedCells.Count - 1];
            unusedCells.RemoveAt(unusedCells.Count - 1);
          }
        }
        return result;
      }

      public ListViewCell DequeueReusableCell(string identifier, Anki.UI.IndexPath indexPath) {
        ListViewCell result = null;
        if (_UnusedCells.ContainsKey(identifier)) {
          List<ListViewCell> unusedCells = _UnusedCells[identifier];
          int foundIdx = unusedCells.FindLastIndex(delegate (ListViewCell cell) {
            return cell.IndexPath == indexPath;
          });
          if (foundIdx != -1) {
            result = unusedCells[foundIdx];
            unusedCells.RemoveAt(foundIdx);
          }
          else {
            result = DequeueReusableCell(identifier);
          }
        }
        return result;
      }

      // Cell Layout Properties 

      // In Pixels =(
      [SerializeField]
      private int
        _CellSpacing = 0;
      public int CellSpacing {
        get { return _CellSpacing; }
        set {
          _CellSpacing = value;
          _UpdateLayoutGroup();
        }
      }

      // In Pixels =(
      [SerializeField]
      private int
        _PaddingLeft = 0;
      public int PaddingLeft {
        get { return _PaddingLeft; }
        set {
          _PaddingLeft = value;
          _UpdateLayoutGroup();
        }
      }

      // In Pixels =(
      [SerializeField]
      private int
        _PaddingRight = 0;
      public int PaddingRight {
        get { return _PaddingRight; }
        set {
          _PaddingRight = value;
          _UpdateLayoutGroup();
        }
      }

      // In Pixels =(
      [SerializeField]
      private int
        _PaddingTop = 0;
      public int PaddingTop {
        get { return _PaddingTop; }
        set {
          _PaddingTop = value;
          _UpdateLayoutGroup();
        }
      }

      // In Pixels =(
      [SerializeField]
      private int
        _PaddingBottom = 0;
      public int PaddingBottom {
        get { return _PaddingBottom; }
        set {
          _PaddingBottom = value;
          _UpdateLayoutGroup();
        }
      }
      
      
      // Lifecycle
      
      protected override void OnLoad() {
        base.OnLoad();
        
//        DAS.Debug("ListView", "OnLoad");
        
        // Add Content Game Object
        _ScrollRectContentObject = new GameObject("Content Panel");

        // Setup Rect Transform
        RectTransform rectTransform = _ScrollRectContentObject.AddComponent<RectTransform>();
        rectTransform.SetParent(this.gameObject.transform, false);
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.anchorMin = new Vector2(0.5f, 0.5f);
        rectTransform.anchorMax = new Vector2(0.5f, 0.5f);
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);

        // Add Content Size Fitter
        _ScrollRectContentObject.AddComponent<ContentSizeFitter>();
        
        // Add ScrollRect component
        _ScrollRect = gameObject.AddComponent<ScrollRect>();
        _ScrollRect.movementType = ScrollRect.MovementType.Elastic;
        _ScrollRect.inertia = true;
        _ScrollRect.content = _ScrollRectContentObject.transform as RectTransform;
        
		// Add Mask component
        //gameObject.AddComponent<Mask>();

        // Check if Data Source or Cell Bindings were added as an component
        // Remove from component after setting property
        DataSource = GetComponent<DataSource>();
        CellBinding = GetComponent<ListViewCellBinding>();

        DAS.Debug("ListView", "OnLoadHappened");
      }

      protected void Start() {
        // Want to make sure the List View loads the 1st time
        ReloadData();
      }

      protected void OnDestroy() {
//        DAS.Debug("ListView", "OnDestroy");
        DataSource = null;
        CellBinding = null;
      }


      // Private

      // Data Source & Cell Binding
      private DataSource _DataSource;   
      private ListViewCellBinding _CellBinding;

      // Scroll Directions
      [SerializeField]
      private bool
        _IsHorizontalScrollDirection = false;

      // Default List Alignment
      [SerializeField]
      private ContentViewAlignment _ListAlignment = ContentViewAlignment.Beginning;

      private bool _DidLayoutScrollDirection = false;

      // For convience hold onto instance
      private ScrollRect _ScrollRect;
      private GameObject _ScrollRectContentObject;

      private uint _NumberOfRows = 0;
      private List<ListViewCell> _VisibleCells = new List<ListViewCell>();
      private Dictionary<string, List<ListViewCell>> _UnusedCells = new Dictionary<string, List<ListViewCell>>();

      private IndexPath? _SelectedIndexPath = null;

      // Responsible for laying out cells in the list container
      private void _LayoutListCells() {
        // Create & Layout Cells
        for (uint idx = 0; idx < _NumberOfRows; ++idx) {
          // TODO: Add Support for Sections
          // TODO: Add Reusable Cells feature
          IndexPath idxPath = new IndexPath(0, idx);

          ListViewCell cell = _CreateAndAddCellToListForIndexPath(idxPath);
          DebugUtils.Assert(cell != null);


          _VisibleCells.Add(cell);
          _CellBinding.OnBindListViewCell(cell, idxPath);
          cell.SetIsSelected((_SelectedIndexPath.HasValue && _SelectedIndexPath.Value == idxPath));
        }
          
        LayoutRebuilder.MarkLayoutForRebuild(_ScrollRect.content);          
      }

      // Note: Index Path Must be valid
      private ListViewCell _CreateAndAddCellToListForIndexPath(IndexPath indexPath) {
        DebugUtils.Assert(DataSource != null);
        DebugUtils.Assert(DataSource.IsIndexPathValid(indexPath));
        DebugUtils.Assert(CellBinding != null);
        ListViewCell cell = null;
        // TODO: Add Support for Sections

        cell = _CellBinding.CellForIndexPath(indexPath);
        cell.IndexPath = indexPath;
        // Set transform and position cell
        if (cell != null) {
          if (cell.transform.parent != _ScrollRect.content) {
            cell.transform.SetParent(_ScrollRect.content, false);
            cell.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
          }
          
          LayoutElement layoutElement = cell.GetComponent<LayoutElement>();
          if (layoutElement == null) {
            layoutElement = cell.gameObject.AddComponent<LayoutElement>();
          }
          //If cell's gameobject is inactive, set active -RA
          if (!cell.gameObject.activeSelf) {
            cell.gameObject.SetActive(true);
          }
        }

        return cell;
      }

      // Note: cell != null
      // Note: Index Path Must be valid
      private void UpdateCellAtIndexPath(ListViewCell cell, IndexPath indexPath) {
        // TODO: Add Support for Sections
        // DAS.Debug("ListView._LayoutCellAtIndexPath", "Layout cell: " + indexPath.ToString() + "SelectedIndexPath: " + ((_SelectedIndexPath.HasValue) ? _SelectedIndexPath.ToString() : "NA") );
       
        DebugUtils.Assert(DataSource != null);
        DebugUtils.Assert(DataSource.IsIndexPathValid(indexPath));
        DebugUtils.Assert(cell != null);


        _CellBinding.OnUnbindListViewCell(cell, indexPath);
        _CellBinding.OnBindListViewCell(cell, indexPath);
        cell.SetIsSelected((_SelectedIndexPath.HasValue && _SelectedIndexPath.Value == indexPath));
      }

        
      // Responsible for cleaning up cells in the list container
      private void _DestroyListCells() {
        if ((_VisibleCells == null) || (_DataSource == null))
          return;
        //If the number of new data fields is fewer than the number of visible cells,
        //make sure to set removed cells to inactive.-RA
        bool fewerCells = (_VisibleCells.Count <= _DataSource.NumberOfItems(0));
        for (int k = 0; k < _VisibleCells.Count; ++k) {
          ListViewCell aCell = _VisibleCells[k];
          if (!_UnusedCells.ContainsKey(aCell.ReuseIdentifier)) {
            _UnusedCells.Add(aCell.ReuseIdentifier, new List<ListViewCell>());
          }


          _CellBinding.OnUnbindListViewCell(aCell, aCell.IndexPath);
          _UnusedCells[aCell.ReuseIdentifier].Add(aCell);
          //Set inactive if the datasource has fewer cells than the current Visible cells
          aCell.gameObject.SetActive(fewerCells);
        }
		    
        _VisibleCells.Clear();
      }

      // Respond to DataSource Action methods
      protected virtual void ReceivedOnObjectDataDidUpdate(List<Anki.UI.IndexPath> updatedIndexPaths) {
        // TODO: Add Support for Sections
        for (int idx = 0; idx < updatedIndexPaths.Count; ++idx) {
          if (_IsCellVisible(updatedIndexPaths[idx])) {
            ListViewCell cell = _VisibleCellAtIndexPath(updatedIndexPaths[idx]);
            UpdateCellAtIndexPath(cell, updatedIndexPaths[idx]);
          }
        }
      }
      
      protected virtual void ReceivedOnDataDidUpdate() {
//      DAS.Debug( "ListView._LayoutCellAtIndexPath", "ReceivedOnDataDidUpdate" );
        ReloadData();
      }

      private bool _DependantComponentsAreReadyFlag = false;
      private bool _DependantComponentsAreReady() {
        if (_DependantComponentsAreReadyFlag == false) {
          if (DataSource != null && CellBinding != null) {
            _DependantComponentsAreReadyFlag = (DataSource.isActiveAndEnabled && CellBinding.isActiveAndEnabled);
          }
        }

        return _DependantComponentsAreReadyFlag;
      }

      // Layout ListView's ScrollDirection
      protected void _LayoutListViewScrollDirection() {
        // Clear Preivious settings
        if (_ScrollRectContentObject.GetComponent<VerticalLayoutGroup>() != null) {
          Destroy(_ScrollRectContentObject.GetComponent<VerticalLayoutGroup>());
        }

        if (_ScrollRectContentObject.GetComponent<HorizontalLayoutGroup>() != null) {
          Destroy(_ScrollRectContentObject.GetComponent<HorizontalLayoutGroup>());
        }

        RectTransform rectTransform = _ScrollRectContentObject.GetComponent<RectTransform>();
        ContentSizeFitter contentSizeFitter = _ScrollRectContentObject.GetComponent<ContentSizeFitter>();
    
        // Setup for list scroll direction
        if (!_IsHorizontalScrollDirection) {
          // Vertical List
          // Scroll Direction
          _ScrollRect.horizontal = false;
          _ScrollRect.vertical = true;

          // Set content transform
          rectTransform.anchorMin = new Vector2(0f, 0f);
          rectTransform.anchorMax = new Vector2(1f, 1f);

          // Create Vertical Layout group
          VerticalLayoutGroup layoutGroup = _ScrollRectContentObject.AddComponent<VerticalLayoutGroup>();
          layoutGroup.padding.left = _PaddingLeft;
          layoutGroup.padding.right = _PaddingRight;
          layoutGroup.padding.top = _PaddingTop;
          layoutGroup.padding.bottom = _PaddingBottom;
          layoutGroup.spacing = _CellSpacing;
          layoutGroup.childAlignment = TextAnchor.MiddleCenter;
          layoutGroup.childForceExpandWidth = false;
          layoutGroup.childForceExpandHeight = false;

          // Set Fitter
          contentSizeFitter.horizontalFit = ContentSizeFitter.FitMode.Unconstrained;
          contentSizeFitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;
        }
        else {
          // Horizontal List
          // Scroll Direction
          _ScrollRect.horizontal = true;
          _ScrollRect.vertical = false;

          rectTransform.anchorMin = new Vector2(0f, 0f);
          rectTransform.anchorMax = new Vector2(1f, 1f);

          // Create Horizontal Layout group
          HorizontalLayoutGroup layoutGroup = _ScrollRectContentObject.AddComponent<HorizontalLayoutGroup>();
          layoutGroup.padding.left = _PaddingLeft;
          layoutGroup.padding.right = _PaddingRight;
          layoutGroup.padding.top = _PaddingTop;
          layoutGroup.padding.bottom = _PaddingBottom;
          layoutGroup.spacing = _CellSpacing;
          layoutGroup.childAlignment = TextAnchor.MiddleRight;
          layoutGroup.childForceExpandWidth = false;
          layoutGroup.childForceExpandHeight = false;

          // Set Fitter
          contentSizeFitter.horizontalFit = ContentSizeFitter.FitMode.PreferredSize;

          // TODO: Need to create cells for this fitmode? Unconstrained
          contentSizeFitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;
        }

        // Update Alignment
        this.ListAlignment = _ListAlignment;
        _DidLayoutScrollDirection = true;
      }

      protected void _UpdateLayoutGroup() {
        // Check if the layout needs to updated
        if (!_DidLayoutScrollDirection)
          return;

        if (!_IsHorizontalScrollDirection) {
          // Vertical List
          VerticalLayoutGroup layoutGroup = _ScrollRectContentObject.GetComponent<VerticalLayoutGroup>();
          layoutGroup.padding.left = _PaddingLeft;
          layoutGroup.padding.right = _PaddingRight;
          layoutGroup.padding.top = _PaddingTop;
          layoutGroup.padding.bottom = _PaddingBottom;
          layoutGroup.spacing = _CellSpacing;
        }
        else {
          // Horizontal List
          HorizontalLayoutGroup layoutGroup = _ScrollRectContentObject.GetComponent<HorizontalLayoutGroup>();
          layoutGroup.padding.left = _PaddingLeft;
          layoutGroup.padding.right = _PaddingRight;
          layoutGroup.padding.top = _PaddingTop;
          layoutGroup.padding.bottom = _PaddingBottom;
          layoutGroup.spacing = _CellSpacing;
        }
      }

    }
  }
}
