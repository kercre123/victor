using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.EventSystems;

[ExecuteInEditMode]
public class ActionButton : MonoBehaviour, IPointerDownHandler, IPointerUpHandler
{
	[SerializeField] protected int otherActions = 3;
	[SerializeField] protected GameObject children;
	[SerializeField] protected float holdTime = 2f;
	[SerializeField] protected float speed = 1500f;

	protected float timePressed = 0f;
	protected float offset = 0f;
	protected float velocity = 0f;
	protected IEnumerator pressedInvocation;

	public void OnPointerDown( PointerEventData eventData )
	{
		timePressed = Time.time;

		pressedInvocation = OnPressed();
		StartCoroutine( pressedInvocation );
	}

	public void OnPointerUp( PointerEventData eventData )
	{
		if( pressedInvocation != null )
		{
		    StopCoroutine( pressedInvocation );
		}
	}

	protected IEnumerator OnPressed()
	{
		while( timePressed + holdTime > Time.time )
		{
			yield return null;
		}

		AnimateInChildren();
	}

	protected void AnimateInChildren()
	{
		children.SetActive( true );

		StartCoroutine( _AnimateInChildren() );
	}

	protected IEnumerator _AnimateInChildren()
	{
		Vector3 localPosition;

		while( Mathf.Abs( children.transform.localPosition.y + ( velocity * Time.deltaTime ) ) < Mathf.Abs( -offset * ( children.transform.childCount + 1.5f ) ) )
		{
			localPosition = children.transform.localPosition;
			localPosition.y += velocity * Time.deltaTime;
			children.transform.localPosition = localPosition;

			yield return null;
		}

		localPosition = children.transform.localPosition;
		localPosition.y = -offset * ( children.transform.childCount + 1.5f );
		children.transform.localPosition = localPosition;
	}

	protected void Awake()
	{
		Image image = gameObject.GetComponent<Image>();
		
		offset = -image.rectTransform.rect.size.y;
		velocity = speed;

		if( image.rectTransform.pivot.y > 0 )
		{
			offset = image.rectTransform.rect.size.y;
			velocity *= -1f;
		}

		if( children != null )
		{
			children.SetActive( false );
		}
	}

	// spawn children in editor time on enable
	protected void OnEnable()
	{
		if( children == null || Application.isPlaying )
		{
			return;
		}

		if( children.transform.childCount != otherActions )
		{
			for( int i = 0; i < children.transform.childCount; ++i )
			{
				DestroyImmediate( children.transform.GetChild( i ).gameObject );

				--i;
			}

			GameObject c = children;
			children = null;

			for( int i = 0; i < otherActions; ++i )
			{
				GameObject go = (GameObject)Instantiate( gameObject );
				DestroyImmediate( go.GetComponent<ActionButton>() );
				DestroyImmediate( go.transform.FindChild( c.name ).gameObject );

				Image image = go.GetComponent<Image>();
				image.name = "Child_" + ( i + 1 ).ToString();

				image.rectTransform.pivot = new Vector2( 0.5f, 0.5f );
				image.rectTransform.anchorMin = new Vector2( 0.5f, 0.5f );
				image.rectTransform.anchorMax = new Vector2( 0.5f, 0.5f );

				image.transform.SetParent( c.transform, false );
				Vector3 localPosition = image.transform.localPosition;
				localPosition.y += offset * ( i + 1 );
				image.transform.localPosition = localPosition;
			}

			children = c;
		}
	}
}
