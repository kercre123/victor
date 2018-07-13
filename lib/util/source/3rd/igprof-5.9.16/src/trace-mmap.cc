#include "trace.h"
#include "atomic.h"
#include "hook.h"
#include "walk-syms.h"
#include <typeinfo>
#include <cxxabi.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cstdarg>
#include <pthread.h>
#include <unistd.h>
#include <cxxabi.h>

// Traps for this profiler module
HOOK(2, int, domunmap, _main,
     (void *addr, size_t len),
     (addr, len),
     "munmap")
HOOK(6, void *, dommap32, _main,
     (void *addr, size_t len, int prot, int flags, int fd, __off_t off),
     (addr, len, prot, flags, fd, off),
     "mmap")
HOOK(6, void *, dommap64, _main,
     (void *addr, size_t len, int prot, int flags, int fd, __off64_t off),
     (addr, len, prot, flags, fd, off),
     "mmap64")

// Data for this trace module
static bool             s_initialized = false;
static bool             s_demangle = false;
static char             *s_demanglehere = 0;
static size_t           s_demanglelen = 0;
static pthread_mutex_t  s_demanglelock = PTHREAD_MUTEX_INITIALIZER;
static IgProfAtomic    s_reporting = 0;

/** Initialise mapping profiling.  Traps various system calls to keep track
    of memory usage, and if requested, leaks.  */
static bool
initialize(void)
{
  if (s_initialized)
    return true;

  s_demanglelen = 1024*1024-32;
  if (! (s_demanglehere = (char *) malloc(s_demanglelen)))
    return false;

  const char    *options = IgTrace::options();
  bool          enable = false;

  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;

    if (! strncmp(options, "mmap", 4))
    {
      enable = true;
      options += 4;
    }
    else if (! strncmp(options, "demangle", 8))
    {
      s_demangle = true;
      options += 8;
    }
    else
      options++;

    while (*options && *options != ',' && *options != ' ')
      options++;
  }

  if (! enable)
    return false;

  if (! IgTrace::initialize())
    return false;

  IgTrace::disable();
  IgHook::hook(domunmap_hook_main.raw);
  IgHook::hook(dommap32_hook_main.raw);
  IgHook::hook(dommap64_hook_main.raw);
  igprof_debug("tracing memory mappings\n");
  s_initialized = true;
  IgTrace::enable();
  return true;
}

//////////////////////////////////////////////////////////////////////
// Mini-sprintf that does not deadlock.  Don't ask :-/
// Snatched from Linux kernel.
static const int ZEROPAD = 1;
static const int LEFT = 2;
static const int SIGN = 4;

static int
xatoi(const char **s)
{
  int i = 0;
  while (isdigit(**s))
    i = i*10 + *((*s)++) - '0';
  return i;
}

static char *
xntoa(char *cur, char *end, unsigned long long num,
      int base, int width, int precision, int flags)
{
  static const char digits[] = "0123456789abcdef";

  if (flags & LEFT)
    flags &= ~ZEROPAD;

  char sign = 0;
  if ((flags & SIGN) && (signed long long) num < 0)
  {
    num = - (signed long long) num;
    sign = '-';
    --width;
  }

  int i = 0;
  char tmp[66];
  if (num == 0)
    tmp[i++] = '0';
  else
    do
    {
      lldiv_t x = lldiv(num, base);
      tmp[i++] = digits[x.rem];
      num = x.quot;
    } while (num != 0);

  if (i > precision)
    precision = i;

  width -= precision;
  if (! (flags & (ZEROPAD|LEFT)))
    for ( ; --width >= 0; ++cur)
      if (cur < end)
        *cur = ' ';

  if (sign)
  {
    if (cur < end)
      *cur = sign;
    ++cur;
  }

  if (! (flags & LEFT))
  {
    char c = (flags & ZEROPAD) ? '0' : ' ';
    for ( ; --width >= 0; ++cur)
      if (cur < end)
        *cur = c;
  }

  for ( ; i <= --precision; ++cur)
    if (cur < end)
      *cur = '0';

  for ( ; --i >= 0; ++cur)
    if (cur < end)
      *cur = tmp[i];

  for ( ; --width >= 0; ++cur)
    if (cur < end)
      *cur = ' ';

  return cur;
}

static int
xsprintf(char *buf, size_t len, const char *format, ...)
{
  va_list       args;
  char          *cur = buf;
  char          *end = buf + len;

  va_start(args, format);
  for (; *format; ++format)
  {
    if (*format != '%')
    {
      if (cur < end)
        *cur = *format;
      ++cur;
      continue;
    }

    int flags = 0;
    ++format;
    if (*format == '-')
      ++format, flags |= LEFT;

    int width = -1;
    if (isdigit(*format))
      width = xatoi(&format);

    int precision = -1;
    if (*format == '.')
    {
      ++format;
      precision = xatoi(&format);
      if (precision < 0)
        precision = 0;
    }

    int qualifier = -1;
    if (*format == 'l' || *format == 'L')
      qualifier = *format++;

    switch (*format)
    {
      // case 'c':
    case 's':
      {
        const char *s = va_arg(args, const char *);
        if ((unsigned long) s < 4096)
          s = "(nil)";

        int len = strnlen(s, precision);
        if (! (flags & LEFT))
          for ( ; len < width; ++cur, --width)
            if (cur < end)
              *cur = ' ';

        for (int i = 0; i < len; ++cur, ++s, ++i)
          if (cur < end)
            *cur = *s;

        for ( ; len < width; ++cur, --width)
          if (cur < end)
            *cur = ' ';

        continue;
      }

    case 'p':
      if (width == -1)
      {
        width = 2*sizeof(void *) + 2;
        flags |= ZEROPAD;
      }

      width -= 2;
      if (cur < end)
        *cur = '0';
      ++cur;
      if (cur < end)
        *cur = 'x';
      ++cur;

      cur = xntoa(cur, end, (unsigned long) va_arg(args, void *),
                  16, width, precision, flags);
      continue;

    case 'd':
      cur = xntoa(cur, end,
                  qualifier == 'L' ? va_arg(args, signed long long)
                  : qualifier == 'l' ? va_arg(args, signed long)
                  : va_arg(args, signed int),
                  10, width, precision, flags | SIGN);
      continue;

    case 'u':
      cur = xntoa(cur, end,
                  qualifier == 'L' ? va_arg(args, unsigned long long)
                  : qualifier == 'l' ? va_arg(args, unsigned long)
                  : va_arg(args, unsigned int),
                  10, width, precision, flags);
      continue;

    case 'x':
      cur = xntoa(cur, end,
                  qualifier == 'L' ? va_arg(args, unsigned long long)
                  : qualifier == 'l' ? va_arg(args, unsigned long)
                  : va_arg(args, unsigned int),
                  16, width, precision, flags);
      continue;

    case '%':
      if (cur < end)
        *cur = '%';
      ++cur;
      continue;

      // case 'n':
      // case 'i':
      // case 'o':
      // case 'X':
      // default:
    }
  }
  va_end(args);

  if (len > 0)
  {
    if (cur < end)
      *cur = 0;
    else
      end[-1] = 0;
  }

  return cur - buf;
}

//////////////////////////////////////////////////////////////////////
// Traps for this trace module.
static void
munmapreport(void *addr, size_t len)
{
  void *stack[800];
  int depth = IgHookTrace::stacktrace(stack, sizeof(stack)/sizeof(stack[0]));

  // If it passes filters, walk the stack to print out information.
  if (IgTrace::filter("munmap", stack, depth))
  {
    char        buf[2048];
    const char  *sym = 0;
    const char  *lib = 0;
    long        symoff = 0;
    long        liboff = 0;

    write(2, buf, xsprintf(buf, sizeof(buf),
                           "*** MUNMAP by %.500s [thread %lu pid %ld]:"
                           " address=%p len=%lu\n",
                           IgTrace::program(),
                           (unsigned long) pthread_self(), (long) getpid(),
                           addr, (unsigned long) len,
                           addr, (char *) addr + len));

    pthread_mutex_lock(&s_demanglelock);
    for (int i = 2; i < depth; ++i)
    {
      void *symaddr = stack[i];
      if (IgHookTrace::symbol(symaddr, sym, lib, symoff, liboff))
        symaddr = (void *) ((intptr_t) symaddr - symoff);

      char hexsym[32];
      if (! sym || ! *sym)
      {
        sprintf(hexsym, "@?%p", symaddr);
        sym = hexsym;
      }
      else if (s_demangle && sym[0] == '_' && sym[1] == 'Z')
      {
        int status = 0;
        char *demangled = abi::__cxa_demangle(sym, s_demanglehere, &s_demanglelen, &status);
        if (status == 0 && demangled && *demangled)
          sym = demangled;
        if (demangled && demangled != s_demanglehere)
          // oops, this shouldn't happen, we might hose ourselves.
          s_demanglehere = demangled;
      }
      if (! lib)
        lib = "<unknown library>";

      write(2, buf, xsprintf(buf, sizeof(buf),
                             "  %3d: %-11p %.500s + %d [%.500s + %d]\n",
                             i-1, stack[i], sym, symoff, lib, liboff));
    }
    pthread_mutex_unlock(&s_demanglelock);
  }
}

static void
mmapreport(const char *sz, void *addr, size_t len, int prot, int flags, int fd, __off64_t off, void *ret)
{
  void *stack[800];
  int depth = IgHookTrace::stacktrace(stack, sizeof(stack)/sizeof(stack[0]));

  // If it passes filters, walk the stack to print out information.
  if (IgTrace::filter("mmap", stack, depth))
  {
    char        buf[2048];
    const char  *sym = 0;
    const char  *lib = 0;
    long        symoff = 0;
    long        liboff = 0;

    write(2, buf, xsprintf(buf, sizeof(buf),
                           "*** MMAP%s by %.500s [thread %lu pid %ld]:"
                           " addr=%p len=%lu"
                           " prot=0x%x flags=0x%x fd=%d offset=%Ld => %p\n",
                           sz, IgTrace::program(),
                           (unsigned long) pthread_self(), (long) getpid(),
                           addr, (unsigned long) len,
                           // addr, (addr ? (char *) addr + len : addr),
                           prot, flags, fd, (long long) off,
                           ret));

    pthread_mutex_lock(&s_demanglelock);
    for (int i = 3; i < depth; ++i)
    {
      void *symaddr = stack[i];
      if (IgHookTrace::symbol(symaddr, sym, lib, symoff, liboff))
        symaddr = (void *) ((intptr_t) symaddr - symoff);

      char hexsym[32];
      if (! sym || ! *sym)
      {
        sprintf(hexsym, "@?%p", symaddr);
        sym = hexsym;
      }
      else if (s_demangle && sym[0] == '_' && sym[1] == 'Z')
      {
        int status = 0;
        char *demangled = abi::__cxa_demangle(sym, s_demanglehere, &s_demanglelen, &status);
        if (status == 0 && demangled && *demangled)
          sym = demangled;
        if (demangled && demangled != s_demanglehere)
          // oops, this shouldn't happen, we might hose ourselves.
          s_demanglehere = demangled;
      }
      if (! lib)
        lib = "<unknown library>";

      write(2, buf, xsprintf(buf, sizeof(buf),
                             "  %3d: %-11p %.500s + %d [%.500s + %d]\n",
                             i-2, stack[i], sym, symoff, lib, liboff));
    }
    pthread_mutex_unlock(&s_demanglelock);
  }
}

static int
domunmap(IgHook::SafeData<igprof_domunmap_t> &hook,
         void *addr, size_t len)
{
  if (s_initialized)
  {
    IgProfAtomic newval = IgProfAtomicInc(&s_reporting);

    if (newval == 1)
      munmapreport(addr, len);

    IgProfAtomicDec(&s_reporting);
  }
  return (*hook.chain)(addr, len);
}

static void *
dommap32(IgHook::SafeData<igprof_dommap32_t> &hook,
         void *addr, size_t len, int prot, int flags, int fd, __off_t off)
{
  void *ret = (*hook.chain)(addr, len, prot, flags, fd, off);
  if (s_initialized)
  {
    IgProfAtomic newval = IgProfAtomicInc(&s_reporting);

    if (newval == 1)
      mmapreport("32", addr, len, prot, flags, fd, off, ret);

    IgProfAtomicDec(&s_reporting);
  }
  return ret;
}

static void *
dommap64(IgHook::SafeData<igprof_dommap64_t> &hook,
         void *addr, size_t len, int prot, int flags, int fd, __off64_t off)
{
  void *ret = (*hook.chain)(addr, len, prot, flags, fd, off);
  if (s_initialized)
  {
    IgProfAtomic newval = IgProfAtomicInc(&s_reporting);

    if (newval == 1)
      mmapreport("64", addr, len, prot, flags, fd, off, ret);

    IgProfAtomicDec(&s_reporting);
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////
static bool autoboot __attribute__((used)) = initialize();
