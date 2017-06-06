using UnityEngine;

namespace Cozmo {
  namespace UI {
    public class BaseModal : BaseDialog {
      // Static events
      public delegate void BaseModalHandler(BaseModal modal);

      public static event BaseModalHandler BaseModalOpened;
      public static event BaseModalHandler BaseModalOpenAnimationFinished;
      public static event BaseModalHandler BaseModalClosed;
      public static event BaseModalHandler BaseModalCloseAnimationFinished;

      public event SimpleBaseDialogHandler ModalClosedWithCloseButtonOrOutside;
      public event SimpleBaseDialogHandler ModalClosedWithCloseButtonOrOutsideAnimationFinished;

      public event SimpleBaseDialogHandler ModalForceClosed;
      public event SimpleBaseDialogHandler ModalForceClosedAnimationFinished;

      /// <summary>
      /// If true, creates a full screen button behind all the elements of this
      /// dialog. The button will close this dialog on click.
      /// </summary>
      [SerializeField]
      private bool _CloseDialogOnTapOutside;

      [SerializeField]
      protected CozmoButtonLegacy _OptionalCloseDialogButton;

      // so we can make modals with new buttons in new skin without
      // breaking legacy modals.
      [SerializeField]
      protected CozmoButton _OptionalCloseDialogCozmoButton;

      [SerializeField]
      private bool _PreventForceClose = false;
      public bool PreventForceClose { get { return _PreventForceClose; } }

      public ModalPriorityData PriorityData { get; private set; }

      public bool DimBackground = false;

      protected bool _UserClosedModal = false;
      protected bool _ModalForceClosed = false;

      public void Initialize(ModalPriorityData priorityData, bool? overrideCloseOnTapOutside) {
        PriorityData = priorityData;
        bool shouldCloseOnTapOutside = overrideCloseOnTapOutside.HasValue ?
          overrideCloseOnTapOutside.Value : _CloseDialogOnTapOutside;
        if (shouldCloseOnTapOutside) {
          CreateFullScreenCloseCollider();
        }

        SetupCloseButton();

        base.Initialize();
      }

      public void ForceCloseModal() {
        if (!IsClosed) {
          _ModalForceClosed = true;
          CloseDialog();
        }
      }

      private void CreateFullScreenCloseCollider() {
        GameObject fullScreenButton = UIManager.CreateUIElement(UIManager.Instance.TouchCatcherPrefab,
                                        this.transform);

        // Place the button underneath all the UI in this dialog
        fullScreenButton.transform.SetAsFirstSibling();
        Cozmo.UI.TouchCatcher fullScreenCollider = fullScreenButton.GetComponent<Cozmo.UI.TouchCatcher>();
        fullScreenCollider.OnTouch += (HandleUserClose);
        fullScreenCollider.Initialize("close_view_by_touch_outside_button", DASEventDialogName);
      }

      private void SetupCloseButton() {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.Initialize(HandleUserClose, "default_close_view_button", DASEventDialogName);
        }

        if (_OptionalCloseDialogCozmoButton != null) {
          _OptionalCloseDialogCozmoButton.Initialize(HandleUserClose, "defualt_close_view_button", DASEventDialogName);
        }
      }

      protected virtual void HandleUserClose() {
        _UserClosedModal = true;
        CloseDialog();
      }

      protected override void RaiseDialogOpened() {
        base.RaiseDialogOpened();
        if (BaseModalOpened != null) {
          BaseModalOpened(this);
        }
      }

      protected override void RaiseDialogOpenAnimationFinished() {
        base.RaiseDialogOpenAnimationFinished();
        if (BaseModalOpenAnimationFinished != null) {
          BaseModalOpenAnimationFinished(this);
        }
      }

      protected override void RaiseDialogClosed() {
        base.RaiseDialogClosed();
        if (_UserClosedModal && ModalClosedWithCloseButtonOrOutside != null) {
          ModalClosedWithCloseButtonOrOutside();
        }
        if (_ModalForceClosed && ModalForceClosed != null) {
          ModalForceClosed();
        }
        if (BaseModalClosed != null) {
          BaseModalClosed(this);
        }
      }

      protected override void RaiseDialogCloseAnimationFinished() {
        base.RaiseDialogCloseAnimationFinished();
        if (_UserClosedModal && ModalClosedWithCloseButtonOrOutsideAnimationFinished != null) {
          ModalClosedWithCloseButtonOrOutsideAnimationFinished();
        }
        if (_ModalForceClosed && ModalForceClosedAnimationFinished != null) {
          ModalForceClosedAnimationFinished();
        }
        if (BaseModalCloseAnimationFinished != null) {
          BaseModalCloseAnimationFinished(this);
        }
      }
    }

    [System.Serializable]
    public class ModalPriorityData {
      [SerializeField]
      private ModalPriorityLayer _PriorityLayer = ModalPriorityLayer.VeryLow;

      [SerializeField]
      private uint _PriorityOffset = 0;

      public uint Priority { get { return (uint)_PriorityLayer + _PriorityOffset; } }

      [SerializeField]
      private LowPriorityModalAction _LowPriorityAction = LowPriorityModalAction.Queue;
      public LowPriorityModalAction LowPriorityAction { get { return _LowPriorityAction; } }

      [SerializeField]
      private HighPriorityModalAction _HighPriorityAction = HighPriorityModalAction.Stack;
      public HighPriorityModalAction HighPriorityAction { get { return _HighPriorityAction; } }

      public ModalPriorityData() {
        _PriorityLayer = ModalPriorityLayer.Low;
        _PriorityOffset = 0;
        _LowPriorityAction = LowPriorityModalAction.CancelSelf;
        _HighPriorityAction = HighPriorityModalAction.Stack;
      }

      public ModalPriorityData(ModalPriorityLayer priorityLayer, uint priorityOffset,
                               LowPriorityModalAction lowPriorityAction, HighPriorityModalAction highPriorityAction) {
        _PriorityLayer = priorityLayer;
        _PriorityOffset = priorityOffset;
        _LowPriorityAction = lowPriorityAction;
        _HighPriorityAction = highPriorityAction;
      }

      public static ModalPriorityData CreateSlightlyHigherData(ModalPriorityData basePriorityData) {
        return new ModalPriorityData(basePriorityData._PriorityLayer,
                                     basePriorityData._PriorityOffset + 1,
                                     basePriorityData._LowPriorityAction,
                                     basePriorityData._HighPriorityAction);
      }
    }

    public enum ModalPriorityLayer {
      VeryHigh = 4000,
      High = 3000,
      Low = 2000,
      VeryLow = 1000
    }

    public enum LowPriorityModalAction {
      Queue,
      CancelSelf
    }

    public enum HighPriorityModalAction {
      Queue,
      Stack,
      ForceCloseOthersAndOpen
    }
  }
}
