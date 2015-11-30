using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;

namespace Anki
{
  namespace UI
  {
    [RequireComponent(typeof(CanvasGroup))]

    public class View : Anki.BaseBehaviour
    {

      public static new View CreateInstance()
      {
        return CreateInstance(null);
      }

      public static View CreateInstance(string name)
      {
        View instance = new GameObject( (name == null) ? "View" : name ).AddComponent<View>();

        RectTransform rectTransform = instance.gameObject.GetComponent<RectTransform>();
        if (rectTransform == null) {
          rectTransform = instance.gameObject.AddComponent<RectTransform>();
        }
        rectTransform.anchorMin = Vector2.zero;
        rectTransform.anchorMax = new Vector2(1.0f, 1.0f);
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);

        return instance;
      }

      // TODO: Need method to set an Image as the Background Image
      public Image BackgroundImage
      {
        get { return gameObject.GetComponent<Image>(); }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// Managing the View Hierarchy

      /// <summary>
      /// Gets the superview.
      /// </summary>
      /// <value>The superview.</value>
      public View Superview { get; private set; }

      /// <summary>
      /// Gets the subviews.
      /// </summary>
      /// <value>The subviews.</value>
      public List<View> Subviews { get; private set; }


      /// <summary>
      /// Adds the subview.
      /// </summary>
      /// <param name="addSubview">Add subview.</param>
      public virtual void AddSubview(View addSubview)
      {
        if (addSubview == null) {
          DAS.Warn("Anki.UI.View", "AddSubview is null");
          return;
        }
        bool hadSuperView = addSubview.Superview; 
        if (hadSuperView) {
          addSubview.WillMoveToSuperview(this);
        }
        addSubview.gameObject.SetActive(true);
        addSubview.transform.SetParent(this.transform, false);

        // FIXME Not sure we want to set the transform, it takes away controll from the addView to set it.
        // However after the Parent is set it can modify the transform?  At least that seems like a problem
        // in the editor
        RectTransform rectTransform = addSubview.transform as RectTransform;
        rectTransform.offsetMin = Vector2.zero;
        rectTransform.offsetMax = Vector2.zero;
        rectTransform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
    		rectTransform.anchoredPosition3D = new Vector3 (0, 0, 0); // raul: Z was being initialized to horrendous values

        if (hadSuperView) {
          addSubview.DidMoveToSuperview();
        }
        DidAddSubview(addSubview);
      }

      /// <summary>
      /// Brings the subview to front.
      /// </summary>
      /// <param name="subview">Subview.</param>
      public virtual void BringSubviewToFront(View subview)
      {
        subview.transform.SetAsLastSibling();
      }

      /// <summary>
      /// Sends the subview to back.
      /// </summary>
      /// <param name="subview">Subview.</param>
      public virtual void SendSubviewToBack(View subview)
      {
        subview.transform.SetAsFirstSibling();
      }

      /// <summary>
      /// Removes from superview.
      /// </summary>
      public virtual void RemoveFromSuperview()
      {
        if (Superview != null) {
          Superview.WillRemoveSubview(this);
          transform.SetParent(null);
        }
      }

      /// <summary>
      /// Inserts the index of the subview at.
      /// </summary>
      /// <param name="subview">Subview.</param>
      /// <param name="atIndex">At index.</param>
      public virtual void InsertSubviewAtIndex(View subview, int atIndex)
      {
        AddSubview(subview);
        subview.transform.SetSiblingIndex(atIndex);
      }

      /// <summary>
      /// Inserts the subview above subview.
      /// </summary>
      /// <param name="subview">Subview.</param>
      /// <param name="aboveSubview">Above subview.</param>
      public virtual void InsertSubviewAboveSubview(View subview, View aboveSubview)
      {
        int idx = Subviews.FindIndex(view => view == aboveSubview);
        if (idx != -1) {
          AddSubview(subview);
          subview.transform.SetSiblingIndex(idx);
        }
      }

      /// <summary>
      /// Inserts the subview below subview.
      /// </summary>
      /// <param name="subview">Subview.</param>
      /// <param name="belowSubview">Below subview.</param>
      // FIXME Not working!!
      public virtual void InsertSubviewBelowSubview(View subview, View belowSubview)
      {
        int idx = Subviews.FindIndex(view => view == belowSubview);

        DAS.Debug("View", "InsertSubviewBelowSubview idx: " + idx + " - Subviews " + Subviews.ToString());

        if (idx != -1) {
          AddSubview(subview);
          subview.transform.SetSiblingIndex(idx + 1);
        }
      }

      /// <summary>
      /// Exchanges the index of the subview at.
      /// </summary>
      /// <param name="subview">Subview.</param>
      /// <param name="subviewAtIndex">Subview at index.</param>
      public virtual void ExchangeSubviewAtIndex(View subview, int subviewAtIndex)
      {
        if (Subviews.Count > subviewAtIndex) {
          View previousView = Subviews[subviewAtIndex];
          previousView.RemoveFromSuperview();
          Subviews.RemoveAt(subviewAtIndex);

          AddSubview(subview);
          subview.transform.SetSiblingIndex(subviewAtIndex);
        }
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// Observing View-Related Changes
     
      /// <summary>
      /// Dids the add subview.
      /// </summary>
      /// <param name="subview">Subview.</param>
      public virtual void DidAddSubview(View subview)
      {
        
      }

      /// <summary>
      /// Wills the remove subview.
      /// </summary>
      /// <param name="subview">Subview.</param>
      public virtual void WillRemoveSubview(View subview)
      {
        
      }

      /// <summary>
      /// Wills the move to superview.
      /// </summary>
      /// <param name="didAddSubview">Did add subview.</param>
      public virtual void WillMoveToSuperview(View didAddSubview)
      {
        
      }

      /// <summary>
      /// Dids the move to superview.
      /// </summary>
      public virtual void DidMoveToSuperview()
      {
        
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// View Life Cycle

      protected override void Awake()
      {
        base.Awake();
        Subviews = new List<View>();
      }
      

    }
  }
}
