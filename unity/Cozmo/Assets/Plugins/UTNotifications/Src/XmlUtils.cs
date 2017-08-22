#if UNITY_EDITOR
using System.Xml;

namespace UTNotifications
{
    public sealed class XmlUtils
    {
        public static XmlNode FindChildNode(XmlNode parent, string name)
        {
            XmlNode it = parent.FirstChild;
            while (it != null)
            {
                if (it.Name.Equals(name))
                {
                    return it;
                }
                it = it.NextSibling;
            }
            return null;
        }

        public static XmlElement FindElement(out XmlNode previous, XmlNode parent, string name, string attributeName = null, string ns = null, string value = null, string elementNs = null)
        {
            previous = null;

            XmlNode it = parent.FirstChild;
            while (it != null)
            {
                if (it.Name.Equals(name) && it is XmlElement && (attributeName == null || (ns == null && ((XmlElement)it).GetAttribute(attributeName) == value) || (ns != null && ((XmlElement)it).GetAttribute(attributeName, ns) == value)) && (elementNs == null || it.NamespaceURI.Equals(elementNs)))
                {
                    return it as XmlElement;
                }
                previous = it;
                it = it.NextSibling;
            }

            return null;
        }

        public static XmlElement UpdateOrCreateElement(XmlDocument document, XmlNode parent, string name, string attributeName = null, string ns = null, string value = null, string commentText = null, string elementNs = null)
        {
            XmlNode previous;
            XmlElement element = FindElement(out previous, parent, name, attributeName, ns, value);
            if (element == null)
            {
                if (commentText != null)
                {
                    XmlComment comment = document.CreateComment(" " + commentText + " ");
                    parent.AppendChild(comment);
                }

                if (elementNs != null)
                {
                    element = document.CreateElement(name, elementNs);
                }
                else
                {
                    element = document.CreateElement(name);
                }

                if (attributeName != null)
                {
                    if (ns != null)
                    {
                        element.SetAttribute(attributeName, ns, value);
                    }
                    else
                    {
                        element.SetAttribute(attributeName, value);
                    }
                }
                parent.AppendChild(element);
            }

            return element;
        }

        public static void RemoveElement(XmlDocument document, XmlNode parent, string name, string attributeName = null, string ns = null, string value = null, string commentText = null, string elementNs = null)
        {
            XmlNode previous;
            XmlElement element = FindElement(out previous, parent, name, attributeName, ns, value);
            if (element != null)
            {
                if (commentText != null && previous != null && previous is XmlComment && ((XmlComment)previous).Data == " " + commentText + " ")
                {
                    element.ParentNode.RemoveChild(previous);
                }
                element.ParentNode.RemoveChild(element);
            }
        }
    }
}

#endif