#include "trace.h"
#include "hook.h"
#include "walk-syms.h"
#include <typeinfo>
#include <cxxabi.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>

// Traps for this profiler module
HOOK(3, void, dothrow, _main,
     (void *exception, std::type_info *tinfo, void (*dest)(void *)),
     (exception, tinfo, dest),
     "__cxa_throw")

// Data for this trace module
static bool             s_initialized = false;
static bool             s_demangle = false;
static char             *s_demanglehere = 0;
static size_t           s_demanglelen = 0;
static pthread_mutex_t  s_demanglelock = PTHREAD_MUTEX_INITIALIZER;

/** Initialise memory profiling.  Traps various system calls to keep track
    of memory usage, and if requested, leaks.  */
static void
initialize(void)
{
  if (s_initialized) return;
  s_initialized = true;

  s_demanglelen = 1024*1024-32;
  if (! (s_demanglehere = (char *) malloc(s_demanglelen)))
    return;

  const char  *options = IgTrace::options();
  bool        enable = false;

  while (options && *options)
  {
    while (*options == ' ' || *options == ',')
      ++options;

    if (! strncmp(options, "throw", 5))
    {
      enable = true;
      options += 5;
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
    return;

  if (! IgTrace::initialize())
    return;

  IgTrace::disable();
  IgHook::hook(dothrow_hook_main.raw);
  igprof_debug("tracing exceptions thrown\n");
  IgTrace::enable();
}

//////////////////////////////////////////////////////////////////////
// Traps for this trace module.
static void
dothrow(IgHook::SafeData<igprof_dothrow_t> &hook,
	void *exception, std::type_info *tinfo,
	void (*dest)(void *))
{
  void *stack[800];
  int depth = IgHookTrace::stacktrace(stack, sizeof(stack)/sizeof(stack[0]));

  // If it passes filters, walk the stack to print out information.
  if (IgTrace::filter(tinfo->name(), stack, depth))
  {
    char            buf[2048];
    const char      *sym = 0;
    const char      *lib = 0;
    long            symoff = 0;
    long            liboff = 0;

    pthread_mutex_lock(&s_demanglelock);
    const char *tname = (tinfo ? tinfo->name() : 0);
    if (tname && *tname && s_demangle)
    {
      int status = 0;
      char *demangled = abi::__cxa_demangle(tname, s_demanglehere,
					    &s_demanglelen, &status);
      if (status == 0 && demangled && *demangled)
	tname = demangled;
      if (demangled && demangled != s_demanglehere)
	s_demanglehere = demangled;
    }

    write(2, buf, sprintf(buf,
			  "*** THROW by %.500s [thread %lu pid %ld]:\n"
			  " Exception of type %.500s (address %p)\n",
			  IgTrace::program(),
			  (unsigned long) pthread_self(), (long) getpid(),
			  tname, exception));

    // Removing the __extension__ gives a warning which
    // is acknowledged as a language problem in the C++ Standard Core
    // Language Defect Report
    //
    // http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#195
    //
    // since the suggested decision seems to be that the syntax should
    // actually be "Conditionally-Supported Behavior" in some
    // future C++ standard I simply silence the warning.
    IgHookTrace::symbol(__extension__ (void *) dest, sym, lib, symoff, liboff);
    if (sym && *sym && s_demangle)
    {
      int status = 0;
      char *demangled = abi::__cxa_demangle(sym, s_demanglehere, &s_demanglelen, &status);
      if (status == 0 && demangled && *demangled)
	sym = demangled;
      if (demangled && demangled != s_demanglehere)
	s_demanglehere = demangled;
    }
    write(2, buf, sprintf(buf, " Destructor %.500s (%p)\n Stack:\n",
			  (sym ? sym : "unknown function"),
			  __extension__ (void *) dest));


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
      else if (s_demangle)
      {
	int status = 0;
	char *demangled = abi::__cxa_demangle(sym, s_demanglehere, &s_demanglelen, &status);
	if (status == 0 && demangled && *demangled)
	  sym = demangled;
	if (demangled && demangled != s_demanglehere)
	  s_demanglehere = demangled;
      }
      if (! lib)
	lib = "<unknown library>";

      write(2, buf, sprintf(buf,
			    "  %3d: %-10p %.500s %s %ld [%.500s %s %ld]\n",
			    i-1, stack[i], sym, (symoff < 0 ? "-" : "+"),
			    labs(symoff), lib, (liboff < 0 ? "-" : "+"),
			    labs(liboff)));
    }
    pthread_mutex_unlock(&s_demanglelock);
  }

  // Call the actual throw.
  (*hook.chain)(exception, tinfo, dest);
}

//////////////////////////////////////////////////////////////////////
static bool autoboot __attribute__((used)) = (initialize (), true);
