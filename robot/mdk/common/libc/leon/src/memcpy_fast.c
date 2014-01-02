typedef unsigned word;
typedef unsigned long long dword;
typedef unsigned short hword;
typedef unsigned char byte;

#pragma weak __memcpy_fast // this means use ROM Version when available
void *__memcpy_fast( void *dest, const void *src, unsigned count ) {
  void *old_dest;
  
  // get out if count in null or too large (1..32M supported)
  if( !count )
    return dest;
  // get out if count is too large (more than 32M) - return NULL
  // this will probably happen on negative counts
  if( count > 32*1024*1024 )
    return 0;

  // backup old destination ptr (we return that)
  old_dest = dest;

  while( count ) {
  
    // dword aligned, with count >=8 ?
    if(( !(( (word)dest | (word)src )& 7 ))&&( count > 7 )) {
      // copy as many dwords as possible
      while( count > 7 ) {
	#ifdef INSTRUMENT
	  ++n64;
	#endif
	*(dword*)dest = *(dword*)src;
	dest = (void*)( 8 + (word)dest );
	src = (void*)( 8 + (word)src );
	count -= 8;
      }
    }

    // word aligned, with count >=4 ?
    if(( !(( (word)dest | (word)src )& 3 ))&&( count > 3 )) {
	#ifdef INSTRUMENT
	  ++n32;
	#endif
      // copy one word
      *(word*)dest = *(word*)src;
      dest = (void*)( 4 + (word)dest );
      src = (void*)( 4 + (word)src );
      count -= 4;
      // if we got dword alignment, iterate
      if( !(( (word)dest | (word)src )& 7 ))
	continue;
      // if we don't get better, copy all we can as words
      while( count > 3 ) {
	#ifdef INSTRUMENT
	  ++n32;
	#endif
        *(word*)dest = *(word*)src;
        dest = (void*)( 4 + (word)dest );
        src = (void*)( 4 + (word)src );
        count -= 4;
      }
    }

    // halfword aligned, with count >=2 ?
    if(( !(( (word)dest | (word)src )& 1 ))&&( count > 1 )) {
      // copy one halfword
	#ifdef INSTRUMENT
	  ++n16;
	#endif
      *(hword*)dest = *(hword*)src;
      dest = (void*)( 2 + (word)dest );
      src = (void*)( 2 + (word)src );
      count -= 2;
      // if we got word / dword alignment, iterate
      if( !(( (word)dest | (word)src )& 3 ))
	continue;
      // if we don't get better, copy all we can as halfwords
      while( count > 1 ) {
	#ifdef INSTRUMENT
	  ++n16;
	#endif
        *(hword*)dest = *(hword*)src;
        dest = (void*)( 2 + (word)dest );
        src = (void*)( 2 + (word)src );
        count -= 2;
      }
    }

    // byte aligned, with count >= 1 ?
    if( count > 0 ) {
      // copy one byte
      #ifdef INSTRUMENT
	++n8;
      #endif
      *(byte*)dest = *(byte*)src;
      dest = (void*)( 1 + (word)dest );
      src = (void*)( 1 + (word)src );
      --count;
      // if we got halford / word / dword alignment, iterate
      if( !(( (word)dest | (word)src )& 1 ))
	continue;
      // if we don't get better, copy all we can as bytes
      while( count ) {
	#ifdef INSTRUMENT
	  ++n8;
	#endif
        *(byte*)dest = *(byte*)src;
        dest = (void*)( 1 + (word)dest );
        src = (void*)( 1 + (word)src );
        --count;
      }
    }
  }
  
  // return the input destination pointer;
  return old_dest;
}
