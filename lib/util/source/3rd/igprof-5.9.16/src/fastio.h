#ifndef FAST_IO_H
# define FAST_IO_H

# include "macros.h"
# include <unistd.h>
# include <string.h>

/** Wrapper for elementary but very fast formatted I/O. */
class HIDDEN FastIO
{
  static const size_t SIZE = 64*1024;
  int fd_;
  size_t pos_;
  char buf_[SIZE];

public:
  FastIO(int fd)
    : fd_(fd),
      pos_(0)
    {}

  ~FastIO(void)
  {}

  /** Attach to a file descriptor. */
  void attach(int fd)
  {
    fd_ = fd;
  }

  /** Flush out all remaining output. */
  void flush(void)
  {
    write(fd_, buf_, pos_);
    pos_ = 0;
  }

  /** Write @a len characters from @a s. Length is guaranteed to be
      less than SIZE bytes long. */
  FastIO &put_fast(const char *s, size_t len)
  {
    ASSERT(pos_ <= SIZE);
    if (UNLIKELY(SIZE - pos_ < len)) flush();
    ASSERT(SIZE - pos_ >= len);
    memcpy(&buf_[pos_], s, len);
    pos_ += len;
    return *this;
  }

  /** Write @a len characters from @a s, reversing the text as its
      written out. The length is guaranteed to be less than SIZE bytes
      long. */
  FastIO &put_fast_rev(const char *s, size_t len)
  {
    ASSERT(pos_ <= SIZE);
    if (UNLIKELY(SIZE - pos_ < len)) flush();
    ASSERT(SIZE - pos_ >= len);
    while (len > 0)
      buf_[pos_++] = s[--len];
    return *this;
  }

  /** Write length @a N constant string @a s. The length is guaranteed
      to be less than SIZE bytes long. */
  template <unsigned N>
  FastIO &put(const char (&s)[N])
  {
    return put_fast(s, N-1);
  }

  /** Write arbitrary length string @a s, length @a len to output.
      The data is automatically chunked to fit the internal buffer. */
  FastIO &put(const char *s, size_t len)
  {
    while (len)
    {
      size_t n = (len < SIZE - pos_ ? len : SIZE - pos_);
      ASSERT(pos_ <= SIZE);
      ASSERT(SIZE - pos_ >= n);
      memcpy(&buf_[pos_], s, n);
      pos_ += n;
      len -= n;
      if (UNLIKELY(pos_ == SIZE)) flush();
    }
    return *this;
  }

  /** Write out @a val as a simply formatted integer. */
  FastIO &put(int val)
  {
    return put((long long) val);
  }

  /** Write out @a val as a simply formatted integer. */
  FastIO &put(long val)
  {
    return put((long long) val);
  }

  /** Write out @a val as a simply formatted integer. */
  FastIO &put(long long signedVal)
  {
    char buf[128];
    size_t n = 0;
    int sign = 0;
    unsigned long long val = signedVal;
    if (UNLIKELY(signedVal < 0))
    {
      sign = -1;
      val = -signedVal;
    }

    if (val == 0)
      buf[n++] = '0';
    else
    {
      do
      {
	ASSERT(n < sizeof(buf));
	long long quot = val >> 4;
	unsigned char rem = val & 0xf;
        buf[n++] = (rem >= 10 ? 'a' + rem - 10 : '0' + rem);
	val = quot;
      } while (val != 0);
    }

    if (UNLIKELY(sign))
      buf[n++] = '-';

    return put_fast_rev(buf, n);
  }

  /** Write out @a val as a simply formatted integer. */
  FastIO &put(unsigned val)
  {
    return put((unsigned long long) val);
  }

  /** Write out @a val as a simply formatted integer. */
  FastIO &put(unsigned long val)
  {
    return put((unsigned long long) val);
  }

  /** Write out @a val as a simply formatted integer. */
  FastIO &put(unsigned long long val)
  {
    char buf[128];
    size_t n = 0;
    if (val == 0)
      buf[n++] = '0';
    else
    {
      do
      {
	ASSERT(n < sizeof(buf));
	unsigned long long quot = val >> 4;
	unsigned char rem = val & 0xf;
        buf[n++] = (rem >= 10 ? 'a' + rem - 10 : '0' + rem);
	val = quot;
      } while (val != 0);
    }

    return put_fast_rev(buf, n);
  }

  /** Write out pointer @a ptr as a hexadecimal integer. */
  FastIO &put(void *ptr)
  {
    unsigned long long val = (unsigned long long) ptr;
    char buf[128];
    size_t n = 0;
    if (val == 0)
      buf[n++] = '0';
    else
    {
      do
      {
	ASSERT(n < sizeof(buf));
	unsigned long long quot = val >> 4;
	unsigned char rem = val & 0xf;
        buf[n++] = (rem >= 10 ? 'a' + rem - 10 : '0' + rem);
	val = quot;
      } while (val != 0);
    }

    ASSERT(n < sizeof(buf)-2);
    buf[n++] = 'x';
    buf[n++] = '0';
    return put_fast_rev(buf, n);
  }
};

#endif // FAST_IO_H
