using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections.Generic;
using DG.Tweening;
using Cozmo.UI;
using DataPersistence;

public class UIManager : MonoBehaviour {

  private static UIManager _Instance;

  public static UIManager Instance {
    get {
      if (_Instance == null) {
        DAS.Error("UIManager.NullInstance", "Do not access UIManager until start: " + System.Environment.StackTrace);
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        DAS.Error("UIManager.DuplicateInstance", "UIManager Instance already exists");
      }
      _Instance = value;
    }
  }

  public static Vector4 GetAtlasUVs(Sprite graphic) {
    float textureAtlasPixelWidth = graphic.texture.width;
    float xMinNormalizedUV = graphic.textureRect.xMin / textureAtlasPixelWidth;
    float xMaxNormalizedUV = graphic.textureRect.xMax / textureAtlasPixelWidth;

    float textureAtlasPixelHeight = graphic.texture.height;
    float yMinNormalizedUV = graphic.textureRect.yMin / textureAtlasPixelHeight;
    float yMaxNormalizedUV = graphic.textureRect.yMax / textureAtlasPixelHeight;

    return new Vector4(xMinNormalizedUV,
      yMinNormalizedUV,
      xMaxNormalizedUV - xMinNormalizedUV,
      yMaxNormalizedUV - yMinNormalizedUV);
  }

  [SerializeField]
  private Canvas _HorizontalCanvas;

  public Camera MainCamera;

  public EventSystem EventSystemScript;

  [SerializeField]
  private GameObject _TouchCatcherPrefab;

  public GameObject TouchCatcherPrefab {
    get { return _TouchCatcherPrefab; }
  }
  private TouchCatcher _TouchCatcherInstance;
  public TouchCatcher TouchCatcher { get { return _TouchCatcherInstance != null && _TouchCatcherInstance.isActiveAndEnabled ? _TouchCatcherInstance : null; } }

  [SerializeField]
  private BackgroundColorController _BackgroundColorController;
  public BackgroundColorController BackgroundColorController { get { return _BackgroundColorController; } }

  [SerializeField]
  private CanvasGroup _DimBackgroundPrefab;

  private CanvasGroup _DimBackgroundInstance;
  private int _NumModalsDimmingBackground;
  private Sequence _DimBackgroundTweener;

  private bool _IsOpenInProgress = false;
  private List<BaseModal> _OpenModals;
  private bool AreAnyModalsOpen { get { return _OpenModals.Count > 0; } }
  private BaseModal CurrentModal {
    get {
      BaseModal currentModal = null;
      if (AreAnyModalsOpen) {
        currentModal = _OpenModals[_OpenModals.Count - 1];
      }
      return currentModal;
    }
  }

  private List<ModalQueueData> _WaitingForOpenQueue;

  private BaseView _CurrentView;
  private bool IsClosingCurrentView {
    get { return (_CurrentView != null && _CurrentView.IsClosed); }
  }

  private bool _IsOpenViewQueued = false;

  public enum CreationCancelledReason {
    CancelledSelf,
    QueueCleared
  }

  void Awake() {
    Instance = this;
    _OpenModals = new List<BaseModal>();
    _WaitingForOpenQueue = new List<ModalQueueData>();
    _NumModalsDimmingBackground = 0;
    DOTween.Init();
    BaseDialog.BaseDialogCloseAnimationFinished += HandleBaseDialogCloseAnimationFinished;
    BaseModal.BaseModalCloseAnimationFinished += HandleBaseModalCloseAnimationFinished;
    BaseView.BaseViewCloseAnimationFinished += HandleBaseViewCloseAnimationFinished;
  }

  void Start() {
    BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.Yellow);
  }

  private void OnDestroy() {
    if (_DimBackgroundTweener != null) {
      _DimBackgroundTweener.Kill();
    }
    BaseDialog.BaseDialogCloseAnimationFinished -= HandleBaseDialogCloseAnimationFinished;
    BaseModal.BaseModalCloseAnimationFinished -= HandleBaseModalCloseAnimationFinished;
    BaseView.BaseViewCloseAnimationFinished -= HandleBaseViewCloseAnimationFinished;
  }

  #region public functions
  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal instead.
  /// </summary>
  public static GameObject CreateUIElement(MonoBehaviour uiPrefab) {
    return CreateUIElement(uiPrefab.gameObject, Instance._HorizontalCanvas.transform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal instead.
  /// </summary>
  public static GameObject CreateUIElement(MonoBehaviour uiPrefab, Transform parentTransform) {
    return CreateUIElement(uiPrefab.gameObject, parentTransform);
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab) {
    GameObject go = null;
    // INGO: Trying fix for https://ankiinc.atlassian.net/browse/COZMO-1172
    if (_Instance != null && _Instance._HorizontalCanvas != null) {
      go = CreateUIElement(uiPrefab, _Instance._HorizontalCanvas.transform);
    }
    return go;
  }

  /// <summary>
  /// Creates a UI element using a script/prefab that extends from MonoBehavior. 
  /// For BaseModals, use OpenModal instead.
  /// </summary>
  public static GameObject CreateUIElement(GameObject uiPrefab, Transform parentTransform) {
    GameObject newUi = Instantiate(uiPrefab);
    newUi.transform.SetParent(parentTransform, false);
    return newUi;
  }

  /// <summary>
  /// Creates a dialog using a script/prefab that extends from BaseView.
  /// Plays open animations on that dialog by default. 
  /// </summary>
  public static void OpenView(
    BaseView viewPrefab,
    System.Action<BaseView> creationSuccessCallback) {
    _Instance.TryStartOpenView(viewPrefab, creationSuccessCallback);
  }

  public static void CloseCurrentView() {
    _Instance.CloseCurrentViewInternal();
  }

  /// <summary>
  /// Creates a dialog using a script/prefab that extends from BaseModal.
  /// Plays open animations on that dialog by default. 
  /// </summary>
  public static void OpenModal(
    BaseModal modalPrefab,
    ModalPriorityData priorityData,
    System.Action<BaseModal> creationSuccessCallback,
    System.Action<CreationCancelledReason> creationCancelledCallback = null, // TODO: Make required
    bool? overrideBackgroundDim = null,
    bool? overrideCloseOnTouchOutside = null) {

    if (_Instance.ModalOpenOrInQueue(modalPrefab.DASEventDialogName)) {
      return;
    }

    _Instance.OpenModalInternal(modalPrefab.DASEventDialogName, modalPrefab, creationSuccessCallback,
                                creationCancelledCallback, priorityData,
                                overrideBackgroundDim, overrideCloseOnTouchOutside);
  }

  public static void CloseModal(BaseModal modalObject) {
    modalObject.CloseDialog();
  }

  public static void CloseModalImmediately(BaseModal modalObject) {
    modalObject.CloseDialogImmediately();
  }

  public static void CloseAllModals() {
    _Instance.CloseAllModalsInternal(forceCloseByModal: false);
  }

  /// <summary>
  /// Creates a dialog using the AlertModal script.
  /// Plays open animations on that dialog by default. 
  /// </summary>
  public static void OpenAlert(
    AlertModalData alertInitializationData,
    ModalPriorityData priorityData,
    System.Action<AlertModal> creationSuccessCallback = null,
    System.Action<CreationCancelledReason> creationCancelledCallback = null,
    bool? overrideBackgroundDim = null,
    bool? overrideCloseOnTouchOutside = null) {

    if (_Instance.ModalOpenOrInQueue(alertInitializationData.DasEventAlertName)) {
      return;
    }

    AlertModal alertPrefab = AlertModalLoader.Instance.AlertModalPrefab;
    if (alertInitializationData.AlertIcon != null) {
      alertPrefab = AlertModalLoader.Instance.IconAlertModalPrefab;
    }
    else if (string.IsNullOrEmpty(alertInitializationData.DescriptionLocKey)) {
      alertPrefab = AlertModalLoader.Instance.NoTextAlertModalPrefab;
    }
    _Instance.OpenModalInternal(alertInitializationData.DasEventAlertName,
                                alertPrefab,
                                (newAlertModal) => {
                                  AlertModal alertModal = (AlertModal)newAlertModal;
                                  alertModal.InitializeAlertData(alertInitializationData);
                                  if (creationSuccessCallback != null) {
                                    creationSuccessCallback(alertModal);
                                  }
                                },
                                creationCancelledCallback,
                                priorityData,
                                overrideBackgroundDim,
                                overrideCloseOnTouchOutside);
  }

  public static Canvas GetUICanvas() {
    return _Instance._HorizontalCanvas;
  }

  public static void DisableTouchEvents() {
    if (_Instance != null && _Instance.EventSystemScript != null) {
      _Instance.EventSystemScript.gameObject.SetActive(false);
    }
  }

  public static void EnableTouchEvents() {
    if (_Instance != null && _Instance.EventSystemScript != null) {
      _Instance.EventSystemScript.gameObject.SetActive(true);
    }
  }

  public static string GetTopModalName() {
    var openModals = _Instance._OpenModals;
    var openModal = openModals.Count > 0 ? openModals[openModals.Count - 1] : null;
    return openModal != null ? openModal.name : string.Empty;
  }

  public void ShowTouchCatcher(System.Action onTouch = null) {
    if (_TouchCatcherInstance == null) {
      _TouchCatcherInstance = Instantiate(_TouchCatcherPrefab).GetComponent<TouchCatcher>();
      _TouchCatcherInstance.transform.SetParent(_Instance._HorizontalCanvas.transform, false);
    }
    _TouchCatcherInstance.Enable();
    if (onTouch != null) {
      _TouchCatcherInstance.OnTouch += onTouch;
    }
  }

  public void HideTouchCatcher() {
    if (_TouchCatcherInstance != null) {
      _TouchCatcherInstance.Disable();
    }
  }
  #endregion // public functions

  #region private functions
  private void TryStartOpenView(
    BaseView viewPrefab,
    System.Action<BaseView> creationSuccessCallback) {

    if (!_IsOpenViewQueued) {
      if (!IsClosingCurrentView) {
        CloseAllModalsInternal(forceCloseByModal: false);
        if (_CurrentView != null) {
          _IsOpenViewQueued = true;
          _CurrentView.DialogCloseAnimationFinished += () => {
            _CurrentView = null;
            OpenViewNow(viewPrefab, creationSuccessCallback);
          };
          CloseCurrentView();
        }
        else {
          OpenViewNow(viewPrefab, creationSuccessCallback);
        }
      }
      else {
        _IsOpenViewQueued = true;
        _CurrentView.DialogCloseAnimationFinished += () => {
          _CurrentView = null;
          OpenViewNow(viewPrefab, creationSuccessCallback);
        };
      }
    }
    else {
#if UNITY_EDITOR
      throw new System.Exception("UIManager.OpenViewInternal.OpenTwoViews: Tried to call OpenView when another view was already queued! typeToOpen=" + viewPrefab.GetType());
#else
      DAS.Error("UIManager.OpenViewInternal.OpenTwoViews", "Tried to call OpenView when another view was already queued typeToOpen=" + viewPrefab.GetType());
#endif
    }
  }

  private void OpenViewNow(
    BaseView viewPrefab,
    System.Action<BaseView> creationSuccessCallback) {
    _IsOpenViewQueued = false;

    GameObject newView = Instantiate(viewPrefab.gameObject);

    Transform targetCanvas = _Instance._HorizontalCanvas.transform;
    newView.transform.SetParent(targetCanvas, false);

    BaseView viewScript = newView.GetComponent<BaseView>();
    _CurrentView = viewScript;

    if (creationSuccessCallback != null) {
      creationSuccessCallback(viewScript);
    }

    // Check DAS name and play open animations
    viewScript.Initialize();

    SendDasEventForDialogOpen(viewScript);
  }

  private void CloseCurrentViewInternal() {
    if (_CurrentView != null) {
      _CurrentView.CloseDialog();
    }
  }

  private bool ModalOpenOrInQueue(string newModalDasEventName) {
    bool modalOpenOrInQueue = false;
    for (int i = 0; i < _OpenModals.Count; i++) {
      if (_OpenModals[i].DASEventDialogName == newModalDasEventName
          && !_OpenModals[i].IsClosed) {
        modalOpenOrInQueue = true;
        break;
      }
    }
    if (modalOpenOrInQueue) {
      for (int i = 0; i < _WaitingForOpenQueue.Count; i++) {
        if (_WaitingForOpenQueue[i].DasEventDialogName == newModalDasEventName) {
          modalOpenOrInQueue = true;
          break;
        }
      }
    }
    return modalOpenOrInQueue;
  }

  private void OpenModalInternal(
    string dasEventDialogName,
    BaseModal modalPrefab,
    System.Action<BaseModal> creationSuccessCallback,
    System.Action<CreationCancelledReason> creationCancelledCallback,
    ModalPriorityData priorityData,
    bool? overrideBackgroundDim = null,
    bool? overrideCloseOnTouchOutside = null) {

    // TODO Make sure to disallow multiple copies of the same dialog based on das event name
    // Check that it's not already open or in queue

    if (_IsOpenInProgress) {
      Debug.LogError("Queueing Modal because an open is in progress " + dasEventDialogName);
      QueueModal(dasEventDialogName, modalPrefab, creationSuccessCallback, creationCancelledCallback,
                             priorityData, overrideBackgroundDim, overrideCloseOnTouchOutside);
    }
    else if (AreAnyModalsOpen) {
      //  Compare priority value of new modal vs top-most modal on stack
      if (priorityData.Priority < CurrentModal.PriorityData.Priority) {
        if (priorityData.LowPriorityAction == LowPriorityModalAction.Queue) {
          // Queue modal
          QueueModal(dasEventDialogName, modalPrefab, creationSuccessCallback, creationCancelledCallback,
                                 priorityData, overrideBackgroundDim, overrideCloseOnTouchOutside);
        }
        else {
          // Cancel opening the new modal
          if (creationCancelledCallback != null) {
            creationCancelledCallback(CreationCancelledReason.CancelledSelf);
          }
        }
      }
      else {
        if (priorityData.HighPriorityAction == HighPriorityModalAction.Queue) {
          // Queue modal
          QueueModal(dasEventDialogName, modalPrefab, creationSuccessCallback, creationCancelledCallback,
                                 priorityData, overrideBackgroundDim, overrideCloseOnTouchOutside);
        }
        else if (priorityData.HighPriorityAction == HighPriorityModalAction.ForceCloseOthersAndOpen) {
          _IsOpenInProgress = true;
          // Close all modals and then open modal now on top of everyone
          CloseAllModalsInternal(forceCloseByModal: true);
          CreateModalInternal(modalPrefab, creationSuccessCallback, creationCancelledCallback,
                                 priorityData, overrideBackgroundDim, overrideCloseOnTouchOutside);
          _IsOpenInProgress = false;
        }
        else {
          // Open modal now on top of everyone
          CreateModalInternal(modalPrefab, creationSuccessCallback, creationCancelledCallback,
                                 priorityData, overrideBackgroundDim, overrideCloseOnTouchOutside);
        }
      }
    }
    else {
      CreateModalInternal(modalPrefab, creationSuccessCallback, creationCancelledCallback,
                             priorityData, overrideBackgroundDim, overrideCloseOnTouchOutside);
    }
  }

  private void CreateModalInternal(
    BaseModal modalPrefab,
    System.Action<BaseModal> creationSuccessCallback,
    System.Action<CreationCancelledReason> creationCancelledCallback,
    ModalPriorityData priorityData,
    bool? overrideBackgroundDim,
    bool? overrideCloseOnTouchOutside) {

    GameObject newModal = Instantiate(modalPrefab.gameObject);

    Transform targetCanvas = _Instance._HorizontalCanvas.transform;
    newModal.transform.SetParent(targetCanvas, false);

    BaseModal modalScript = newModal.GetComponent<BaseModal>();
    if (creationSuccessCallback != null) {
      creationSuccessCallback(modalScript);
    }

    modalScript.Initialize(priorityData, overrideCloseOnTouchOutside);

    SendDasEventForDialogOpen(modalScript);

    _OpenModals.Add(modalScript);

    bool shouldDimBackground = overrideBackgroundDim.HasValue ? overrideBackgroundDim.Value : modalScript.DimBackground;
    if (shouldDimBackground) {
      DimBackground(modalScript, _Instance._HorizontalCanvas.transform);
    }

    PlaceDimmer();
  }

  private void CloseAllModalsInternal(bool forceCloseByModal) {
    ClearModalQueue();
    // Close modals down the stack
    for (int i = _OpenModals.Count - 1; i >= 0; i--) {
      if (_OpenModals[i] != null) {
        if (forceCloseByModal) {
          if (!_OpenModals[i].PreventForceClose) {
            _OpenModals[i].ForceCloseModal();
          }
        }
        else {
          _OpenModals[i].CloseDialog();
        }
      }
    }
  }

  private void QueueModal(
    string dasEventDialogName,
    BaseModal modalPrefab,
    System.Action<BaseModal> creationSuccessCallback,
    System.Action<CreationCancelledReason> creationCancelledCallback,
    ModalPriorityData priorityData,
    bool? overrideBackgroundDim,
    bool? overrideCloseOnTouchOutside) {
    ModalQueueData queueData = new ModalQueueData(dasEventDialogName, modalPrefab, creationSuccessCallback, creationCancelledCallback,
                                                  overrideBackgroundDim, overrideCloseOnTouchOutside, priorityData);
    uint newModalPriority = priorityData.Priority;
    bool hasQueuedData = false;
    for (int i = 0; i < _WaitingForOpenQueue.Count; i++) {
      if (newModalPriority > _WaitingForOpenQueue[i].PriorityData.Priority) {
        _WaitingForOpenQueue.Insert(i, queueData);
        hasQueuedData = true;
        break;
      }
    }

    if (!hasQueuedData) {
      _WaitingForOpenQueue.Add(queueData);
    }
  }

  private void ClearModalQueue() {
    for (int i = 0; i < _WaitingForOpenQueue.Count; i++) {
      if (_WaitingForOpenQueue[i].CreationCancelledCallback != null) {
        _WaitingForOpenQueue[i].CreationCancelledCallback(CreationCancelledReason.QueueCleared);
      }
    }
    _WaitingForOpenQueue.Clear();
  }

  private void HandleBaseDialogCloseAnimationFinished(BaseDialog dialog) {
    SendDasEventForDialogClose(dialog);
  }

  private void HandleBaseViewCloseAnimationFinished(BaseView view) {
    if (view == _CurrentView) {
      _CurrentView = null;
    }
  }

  private void HandleBaseModalCloseAnimationFinished(BaseModal modal) {
    _OpenModals.Remove(modal);
    TryUnDimBackground(modal);
    OpenNextModalInQueue();
  }

  private void OpenNextModalInQueue() {
    if (!_IsOpenViewQueued && !IsClosingCurrentView && !_IsOpenInProgress &&
        !AreAnyModalsOpen && _WaitingForOpenQueue.Count > 0) {
      ModalQueueData nextData = _WaitingForOpenQueue[0];
      _WaitingForOpenQueue.RemoveAt(0);

      OpenModal(nextData.Prefab, nextData.PriorityData, nextData.CreationSuccessCallback, nextData.CreationCancelledCallback,
             nextData.OverrideBackgroundDim, nextData.OverrideCloseOnTouchOutside);
    }
  }

  private void DimBackground(BaseModal modalObject, Transform targetCanvas) {
    modalObject.DimBackground = true;
    _NumModalsDimmingBackground++;
    if (_NumModalsDimmingBackground == 1) {
      if (_DimBackgroundInstance == null) {
        // First dialog to dim the background, we need to create a dimmer
        // on top of existing ui, if the modal has a specific prefab it wants
        // to use for dimming the background, use that instead
        // (For instance, onboarding background dimming that wants to avoid
        // covering the entire screen)
        GameObject toInstantiate = _DimBackgroundPrefab.gameObject;
        _DimBackgroundInstance = Instantiate(toInstantiate).GetComponent<CanvasGroup>();
        _DimBackgroundInstance.transform.SetParent(targetCanvas, false);
        _DimBackgroundInstance.alpha = 0;
      }

      if (_DimBackgroundTweener != null) {
        _DimBackgroundTweener.Kill();
      }
      _DimBackgroundTweener = DOTween.Sequence();
      _DimBackgroundTweener.Append(_DimBackgroundInstance.DOFade(1, UIDefaultTransitionSettings.Instance.FadeInTransitionDurationSeconds)
                                   .SetEase(UIDefaultTransitionSettings.Instance.FadeInEasing));
    }
  }

  private void TryUnDimBackground(BaseModal modalObject) {
    if (modalObject.DimBackground) {
      _NumModalsDimmingBackground--;
      if (_NumModalsDimmingBackground <= 0) {
        // Destroy the dimmer when no dialogs want it anymore
        _NumModalsDimmingBackground = 0;
        if (_DimBackgroundInstance != null) {
          if (_DimBackgroundTweener != null) {
            _DimBackgroundTweener.Kill();
          }
          _DimBackgroundTweener = DOTween.Sequence();
          _DimBackgroundTweener.Append(_DimBackgroundInstance.DOFade(0, UIDefaultTransitionSettings.Instance.FadeOutTransitionDurationSeconds)
                                       .SetEase(UIDefaultTransitionSettings.Instance.FadeOutEasing));
          _DimBackgroundTweener.AppendCallback(() => Destroy(_DimBackgroundInstance.gameObject));
        }
      }
    }
    PlaceDimmer();
  }

  private void PlaceDimmer() {
    // Find the top-most modal that dims the background
    if (_DimBackgroundInstance != null) {
      for (int i = _OpenModals.Count - 1; i >= 0; i--) {
        if (_OpenModals[i].DimBackground) {
          // Set dimmer placement in hierarchy just beneath that modal
          int targetIndex = _OpenModals[i].transform.GetSiblingIndex();
          int currentDimmerIndex = _DimBackgroundInstance.transform.GetSiblingIndex();
          if (currentDimmerIndex < targetIndex) {
            _DimBackgroundInstance.transform.SetSiblingIndex(targetIndex - 1);
          }
          else {
            _DimBackgroundInstance.transform.SetSiblingIndex(targetIndex);
          }
          break;
        }
      }
    }
  }

  private static void SendDasEventForDialogOpen(BaseDialog newDialog) {
    string eventOpenId = DASUtil.FormatViewTypeForOpen(newDialog);
    SendDialogDasEvent(eventOpenId, newDialog);
  }

  private static void SendDasEventForDialogClose(BaseDialog closedDialog) {
    string eventClosedId = DASUtil.FormatViewTypeForClose(closedDialog);
    SendDialogDasEvent(eventClosedId, closedDialog);
  }

  private static void SendDialogDasEvent(string eventId, BaseDialog dialogToSend) {
    DAS.Event(eventId, dialogToSend.DASEventDialogName);
  }
  #endregion // private functions

  private class ModalQueueData {
    public readonly BaseModal Prefab;
    public readonly System.Action<BaseModal> CreationSuccessCallback;
    public readonly System.Action<CreationCancelledReason> CreationCancelledCallback;
    public readonly bool? OverrideBackgroundDim;
    public readonly bool? OverrideCloseOnTouchOutside;
    public readonly ModalPriorityData PriorityData;
    public readonly string DasEventDialogName;

    public ModalQueueData(string dasEventName, BaseModal prefab, System.Action<BaseModal> createSuccessCallback,
                          System.Action<CreationCancelledReason> creationCancelledCallback,
                          bool? overrideBackgroundDim, bool? overrideCloseOnTouchOutside,
                          ModalPriorityData priorityData) {
      this.DasEventDialogName = dasEventName;
      this.Prefab = prefab;
      this.CreationSuccessCallback = createSuccessCallback;
      this.CreationCancelledCallback = creationCancelledCallback;
      this.OverrideBackgroundDim = overrideBackgroundDim;
      this.OverrideCloseOnTouchOutside = overrideCloseOnTouchOutside;
      this.PriorityData = priorityData;
    }
  }
}