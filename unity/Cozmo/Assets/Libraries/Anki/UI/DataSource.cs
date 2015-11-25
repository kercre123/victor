using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki;

namespace Anki {
  namespace UI {

    public struct IndexPath
    {
      public uint Section, Item;

      public IndexPath(uint section, uint item)
      {
        this.Section = section;
        this.Item = item;
      }

      public override bool Equals(System.Object obj) {
        if (null == obj) {
          return false;
        }

        if (!(obj is IndexPath))
          return false;

        IndexPath other = (IndexPath)obj;

        return (this == other);
      }

      public override int GetHashCode()
      {
        unchecked // Overflow is fine, just wrap
        {
          int prime = 31;
          int hash = 1;
          hash = hash * prime + (int)Section;
          hash = hash * prime + (int)Item;
          return hash;
        }
      }

      public static bool operator ==(IndexPath idx1, IndexPath idx2)
      {
        return ( idx1.Section == idx2.Section && idx1.Item == idx2.Item );
      }

      public static bool operator !=(IndexPath idx1, IndexPath idx2)
      {
        return !(idx1 == idx2);
      }

      public override string ToString()
      {
        return string.Format("[{0}, {1}]", Section, Item);
      }
    }

    public class DataSource : Anki.BaseBehaviour
    {
      // Subset of objects were updated
      public event Action< List<IndexPath> > OnObjectDataDidUpdate;

      // All objects did update
      public event Action OnDataDidUpdate;

      public virtual uint NumberOfSections()
      {
        return 1;
      }

      public virtual uint NumberOfItems(uint section)
      {
        return 0;
      }

      public virtual System.Object ObjectAtIndexPath(IndexPath indexPath)
      {
        return null;
      }

      public virtual string TitleForSection(uint section)
      {
        return "";
      }

      public virtual bool IsIndexPathValid(Anki.UI.IndexPath indexPath)
      {
				return (indexPath.Section < NumberOfSections() && indexPath.Item < NumberOfItems(indexPath.Section));
      }

      ///  Actions

      protected void ObjectDataDidUpdate(List<IndexPath> indexPaths)
      {
        if (OnObjectDataDidUpdate != null) {
          OnObjectDataDidUpdate(indexPaths);
        }
      }

      protected void DataDidUpdate()
      {
        if (OnDataDidUpdate != null) {
          OnDataDidUpdate();
        }
      }

    }
  }
}
