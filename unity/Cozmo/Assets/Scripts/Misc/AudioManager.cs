using UnityEngine;
using System.Collections;

public class AudioManager : MonoBehaviour
{
	private static AudioManager instance = null;

	[SerializeField] private AudioSource[] audioSources;

	public enum Source
	{
		UI = 0,
		Robot,
		Gameplay,
		Notification,
		Count
	}

	private static AudioSource OneShot { get { return instance.audioSources[(int)Source.Count]; } }
	public static AudioSource CozmoVisionLoop { get { return instance != null ? instance.audioSources[(int)Source.Count + 1] : null; } }

	void Awake()
	{
		if( instance != null )
		{
			GameObject.Destroy( gameObject );
			return;
		}

		instance = this;
	}

	private void OnDestroy()
	{
		if( instance == this ) instance = null;
	}

	public static void PlayOneShot( AudioClip clip, float delay = 0f, float pitch = 1f, float volume = 1f )
	{
		if( instance == null )
		{
			Debug.LogWarning( "AudioManager is null" );
			return;
		}

		instance.StartCoroutine( instance._PlayAudioClip( clip, delay, true, false, false, OneShot, pitch, volume ) );
	}

	private static void PlayAudioClip( AudioClip clip, float delay, bool oneShot, bool loop, bool deferred, Source source, float pitch, float volume )
	{
		if( instance == null )
		{
			Debug.LogWarning( "AudioManager is null" );
			return;
		}
		
		AudioSource audio = instance.audioSources[(int)source];
		
		if( audio == null || ( deferred && audio.isPlaying ) || ( audio.clip == clip && audio.isPlaying ) ) return;
		
		instance.StartCoroutine( instance._PlayAudioClip( clip, delay, false, loop, deferred, audio, pitch, volume ) );
	}

	public static void PlayAudioClip( AudioClip clip, float delay = 0f, bool loop = false, bool deferred = false, Source source = Source.UI, float pitch = 1f, float volume = 1f )
	{
		PlayAudioClip( clip, delay, false, loop, deferred, source, pitch, volume );
	}
	
	private IEnumerator _PlayAudioClip( AudioClip clip, float delay, bool oneShot, bool loop, bool deferred, AudioSource audioSource, float pitch, float volume )
	{
		yield return new WaitForSeconds( delay );

		audioSource.pitch = pitch;
		audioSource.volume = volume;

		if( !oneShot )
		{
			audioSource.loop = loop;
			audioSource.clip = clip;
			audioSource.Stop(); 
			audioSource.Play();
		}
		else
		{
			audioSource.loop = false;
			audioSource.PlayOneShot( clip );
		}
	}
	
	public static void PlayAudioClips( AudioClip[] clips, float initalDelay = 0f, float delayBetweenClips = 0.05f, bool oneShot = false, bool deferred = false, 
	                                  Source source = Source.UI, float pitch = 1f, float volume = 1f )
	{
		if( instance == null )
		{
			Debug.LogWarning( "AudioManager is null" );
			return;
		}

		if( clips.Length > 0 )
		{
			PlayAudioClip( clips[0], initalDelay, oneShot, false, deferred, source, pitch, volume );
			
			for( int i = 1; i < clips.Length; ++i )
			{
				PlayAudioClip( clips[i], clips[i-1].length + delayBetweenClips, oneShot, false, deferred, source, pitch, volume );
			}
		}
	}

	public static void Stop( AudioClip clip )
	{
		if( instance == null )
		{
			Debug.LogWarning( "AudioManager is null" );
			return;
		}

		for( int i = 0; i < instance.audioSources.Length; ++i )
		{
			if( instance.audioSources[i].clip == clip )
			{
				instance.audioSources[i].Stop();
				instance.audioSources[i].clip = null;
			}
		}
	}

	public static void Stop( Source source = Source.Count )
	{
		if( instance == null )
		{
			Debug.LogWarning( "AudioManager is null" );
			return;
		}

		if( (int)source != (int)Source.Count && (int)source < instance.audioSources.Length )
		{
			instance.audioSources[(int)source].Stop();
			instance.audioSources[(int)source].volume = 0f;
		}
		else // stop all sounds
		{
			for( int i = 0; i < instance.audioSources.Length; ++i )
			{
				instance.audioSources[i].Stop();
				instance.audioSources[i].volume = 0f;
			}
		}
	}
}
