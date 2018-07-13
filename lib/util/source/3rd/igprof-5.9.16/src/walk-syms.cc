#include "walk-syms.h"
#include "macros.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#if __linux
# include <execinfo.h>
# include <ucontext.h>
# include <sys/syscall.h>
# if __x86_64__ || __arm__ || __aarch64__
#  define UNW_LOCAL_ONLY
#  include <libunwind.h>
# endif
#endif
#if __APPLE__
extern "C" void _sigtramp(void);
#endif

#if !defined MAP_ANONYMOUS && defined MAP_ANON
# define MAP_ANONYMOUS MAP_ANON
#endif

bool
IgHookTrace::symbol(void *address,
                    const char *&sym,
                    const char *&lib,
                    long &offset,
                    long &liboffset)
{
  sym = lib = 0;
  offset = 0;
  liboffset = (unsigned long) address;

  Dl_info info;
  if (dladdr(address, &info))
  {
    if (info.dli_fname && info.dli_fname[0])
      lib = info.dli_fname;

    if (info.dli_fbase)
      liboffset = (unsigned long) address - (unsigned long) info.dli_fbase;

    if (info.dli_saddr)
      offset = (unsigned long) address - (unsigned long) info.dli_saddr;

    if (info.dli_sname && info.dli_sname[0])
      sym = info.dli_sname;

    return true;
  }

  return false;
}

void *
IgHookTrace::tosymbol(void *address)
{
  Dl_info info;
  return (dladdr(address, &info)
	  && info.dli_fname
	  && info.dli_fname[0]
	  && info.dli_saddr)
    ? info.dli_saddr : address;
}

int
IgHookTrace::stacktrace(void **addresses, int nmax)
{
#if __linux && __i386__
  // Safer assumption for the VSYSCALL_PAGE.
  //
  // This in particular fixes the regression that I spotted
  // in the recent changes for RHEL5. Notice that this
  // assumption is not valid anymore if the program is
  // being profiled under an hypervisor as mentioned here:
  //
  //     http://lkml.indiana.edu/hypermail/linux/kernel/0605.2/0016.html
  //
# define PROBABLY_VSYSCALL_PAGE 0xffffe000
  struct frame
  {
    // Normal frame.
    frame           *ebp;
    void            *eip;
    // Signal frame stuff, put in here by kernel.
    int             signo;
    siginfo_t       *info;
    ucontext_t      *ctx;
  };
  register frame    *ebp __asm__ ("ebp");
  register frame    *esp __asm__ ("esp");
  frame             *fp = ebp;
  frame             *first = fp;
  int               depth = 0;

  // Top-most frame ends with null pointer; check the rest is reasonable
  while (depth < nmax && fp >= esp && fp->eip)
  {
    // Add this stack frame.  The return address is the
    // instruction immediately after the "call".  The call
    // instruction itself is 4 or 6 bytes; we guess 4.
    addresses[depth++] = (char *) fp->eip - 4;

    // Recognise signal frames.  We use two different methods
    // depending on the linux kernel version.
    //
    // For the "old" kernels / systems we check the instructions
    // at the caller's return address.  We take it to be a signal
    // frame if we find the signal return code sequence there
    // and the thread register context structure pointer:
    //
    //    mov $__NR_rt_sigreturn, %eax
    //    int 0x80
    //
    // For the "new" kernels / systems the operating system maps
    // a "vsyscall" page at a high address, and it may contain
    // either the above code, or use of the sysenter/sysexit
    // instructions.  We cannot poke at that page so we take the
    // the high address as an indication this is a signal frame.
    // (http://www.trilithium.com/johan/2005/08/linux-gate/)
    // (http://manugarg.googlepages.com/systemcallinlinux2_6.html)
    //
    // If we don't recognise the signal frame correctly here, we
    // lose one stack frame: signal delivery is not a call so
    // when the signal handler is entered, ebp still points to
    // what it was just before the signal.
    unsigned char *insn = (unsigned char *) fp->eip;
    if (insn
	&& insn[0] == 0xb8 && insn[1] == __NR_rt_sigreturn
	&& insn[5] == 0xcd && insn[6] == 0x80
	&& fp->ctx)
    {
      void *retip = (void *) fp->ctx->uc_mcontext.gregs[REG_EIP];
      if (depth < nmax)
	addresses[depth++] = retip;

      if ((fp = (frame *)fp->ctx->uc_mcontext.gregs[REG_EBP])
	  && (unsigned long) fp < PROBABLY_VSYSCALL_PAGE
	  && (unsigned long) retip > PROBABLY_VSYSCALL_PAGE
	  && fp > first)
      {
	// __kernel_vsyscall stack on system call exit is
	// [0] %ebp, [1] %edx, [2] %ecx, [3] return address.
	if (depth < nmax)
	  addresses[depth++] = ((void **) fp)[3];
	fp = fp->ebp;

	// It seems the frame _above_ __kernel_syscall (the
	// syscall implementation in libc, such as __mmap())
	// is essentially frame-pointer-less, so we should
	// find also the call above, but I don't know how
	// to determine how many arguments the system call
	// pushed on stack to call __kernel_syscall short
	// of interpreting the DWARF unwind information :-(
	// So we may lose one level of call stack here.
      }

      if ((unsigned long) fp >= PROBABLY_VSYSCALL_PAGE)
	fp = 0;
    }

    // Otherwise it's a normal frame, process through frame pointer.
    // Allow at most 256kB displacement per frame.
    else if (abs((char *) fp - (char *) fp->ebp) < 256*1024)
      fp = fp->ebp;
    else
      break;
  }

  return depth;
#elif __linux && __x86_64__ || __arm__ || __aarch64__
  return unw_backtrace(addresses, nmax);
#if 0 // Debug code for tracking unwind failures.
  if (addresses[depth-1] != (void *) 0x40cce9)
  {
    char buf[512];
    write(2, buf, sprintf(buf, "UWTRACE %d\n", depth));
    for (int i = 0; i < depth; ++i)
    {
      const char *sym = 0, *lib = 0;
      long off = 0, liboff = 0;
      symbol(addresses[i], sym, lib, off, liboff);
      write(2, buf, sprintf(buf, " #%-3d 0x%016lx %s %s %lx [%s %s %lx]\n",
			    i, (unsigned long) addresses[i],
			    sym ? sym : "(unknown)",
			    off < 0 ? "-" : "+", labs(off),
			    lib ? lib : "(unknown)",
			    liboff < 0 ? "-" : "+", labs(liboff)));
    }
    write(2, buf, sprintf(buf, " UWEND\n"));
  }
#endif
#elif __APPLE__ && __ppc__
  struct frame { frame *sp; void *cr; char *lr; };
  char                *sigtramplow = (char *) &_sigtramp;
  char                *sigtramphi  = (char *) sigtramplow + 256;
  register frame      *sp __asm__ ("sp");
  register char       *lr __asm__ ("lr");
  frame               *fp = sp;
  char                *entry = lr;
  int                 depth = 0;

  while (depth < nmax && entry)
  {
    // LR points to the instruction after call, so step back.
    addresses[depth++] = entry - 4;

    // Check next one is a valid frame.
    frame *next = fp->sp;
    if (next <= fp || next <= sp)
      break;

    // Get and handle previous frame's call address.  Signal
    // frames are detected by being in sigtramp() and need
    // special handling.  The offset for pre-signal SP is
    // somewhat magic.
    if (entry >= sigtramplow && entry <= sigtramphi)
    {
      fp = *(frame **) ((char *) next + 156);
      entry = *(char **) ((char *) next + 144);
    }
    else
    {
      fp = next;
      entry = fp->lr;
    }
  }

  return depth;
#else
  return 0;
#endif
}
