typedef unsigned word;
typedef unsigned long long dword;
typedef unsigned short hword;
typedef unsigned char byte;

#pragma weak __memset_fast // this means use ROM Version when available
void *__memset_fast( void *dest, int value, unsigned count ) {
  register union {
    struct {
      word w1;
      word w2;
    };
    dword d;
  } val;

  val.w1 = value & 0xFF;
  value = (int)dest;
  // first write - byte?
  if( count ) {
    // on byte-aligned destination: write one byte
    if( (unsigned)dest & 1 ) {
      #ifdef INSTRUMENT
	++n8;
      #endif
      *(byte*)dest = (byte)val.w1;
      dest = (void*)( 1 + (unsigned)dest );
      --count;
    }
    if( count >= 2 ) {
      val.w1 &= 0xFF;
      val.w1 |= val.w1 << 8;
      // on hword-aligned destination: write one halfword
      if( (unsigned)dest & 2 ) {
	#ifdef INSTRUMENT
	  ++n16;
	#endif
        *(hword*)dest = (hword)val.w1;
        dest = (void*)( 2 + (unsigned)dest );
	count -= 2;
      }
      if( count >= 4 ) {
	val.w1 |= val.w1 << 16;
        // on word-aligned destination: write one word
	if( (unsigned)dest & 4 ) {
	  #ifdef INSTRUMENT
	    ++n32;
	  #endif
	  *(word*)dest = (word)val.w1;
	  dest = (void*)( 4 + (unsigned)dest );
	  count -= 4;
	}
	if( count >= 8 ) {
	  val.w2 = val.w1;
          // on dword-aligned destination: write as many dwords as possible
	  while( count >= 8 ) {
	    #ifdef INSTRUMENT
	      ++n64;
	    #endif
	    *(dword*)dest = val.d;
	    dest = (void*)( 8 + (unsigned)dest );
	    count -= 8;
	  }
	}
        // if after dword pass we have 4+ bytes, write a word
	if( count >= 4 ) {
	  #ifdef INSTRUMENT
	    ++n32;
	  #endif
	  *(word*)dest = (word)val.w1;
	  dest = (void*)( 4 + (unsigned)dest );
	  count -= 4;
	}
      }
      // if after word pass we have 2+ bytes, write a halfword
      if( count >= 2 ) {
	#ifdef INSTRUMENT
	  ++n16;
	#endif
	*(hword*)dest = (hword)val.w1;
	dest = (void*)( 2 + (unsigned)dest );
	count -= 2;
      }
    }
    // if after halfword pass we have 1+ bytes, write a byte
    if( count ) {
      #ifdef INSTRUMENT
	++n8;
      #endif
      *(byte*)dest = (byte)val.w1;
    }
  }
  
  return (void*)value;
}
