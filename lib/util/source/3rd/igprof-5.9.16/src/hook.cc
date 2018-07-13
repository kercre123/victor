#include "hook.h"
#include "profile.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <cassert>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>

#if __linux
#include <link.h>
#endif

#if __APPLE__
#include <mach/mach.h>
#endif

#if __aarch64__
#include <stdint.h>
#endif

#if __i386__
# define TRAMPOLINE_JUMP        5       // jump to hook/old code
# define TRAMPOLINE_SAVED       10      // 5+margin for saved prologue
#elif __x86_64__
# define TRAMPOLINE_JUMP        32      // jump to hook/old code
# define TRAMPOLINE_SAVED       10      // 5+margin for saved prologue
#elif __ppc__
# define TRAMPOLINE_JUMP        16      // jump to hook/old code
# define TRAMPOLINE_SAVED       4       // one prologue instruction to save
#elif __arm__
# define TRAMPOLINE_JUMP        8       // jump to hook/old code (2 instructions)
# define TRAMPOLINE_SAVED       8       // 2 reserved words for possible offsets
#elif __aarch64__
# define TRAMPOLINE_JUMP        4       // jump to hook/old code (short jump)
# define TRAMPOLINE_SAVED       32      // long jump+patch area
#else
# error sorry this platform is not supported
#endif

#define TRAMPOLINE_SIZE (TRAMPOLINE_JUMP+TRAMPOLINE_SAVED+TRAMPOLINE_JUMP)

#if !defined MAP_ANONYMOUS && defined MAP_ANON
# define MAP_ANONYMOUS MAP_ANON
#endif

#if !__linux
# define dlvsym(h,fn,v) dlsym(h,fn)
#endif

//struct modRM to help finding out lenght of instruction
struct modRMBits {
  unsigned char rm:3;
  unsigned char reg:3;
  unsigned char mod:2;
};

union modRMByte {
  unsigned char encoded;
  modRMBits     bits;
};

struct IgHookFindSym {
  struct stat *st;
  const char *fn;
  const char *lib;
  void *sym;
};

/** Allocate a trampoline area into @a ptr.  Returns error code on
    failure, success otherwise.  The memory is allocated into an
    address suitable for single-instruction branches (see @c direct
    argument to #redirect()) if the architecture so requires.  */
static IgHook::Status
allocate(void *&ptr, void *target UNUSED)
{
  // FIXME: a page per trampoline is a bit excessive...
  unsigned int pagesize = getpagesize();
#if __APPLE__ && __ppc__
  // Allocate at end of memory so the address sign extends -- "ba"
  // can only take 24-bit immediate offset plus two zero bits
  // stripped off at the right end.  It can be either in the low 25
  // bits which can be crowded, or 26-bit address that sign extends
  // -- i.e. in high memory.  Thus the top six bits of the address
  // are required to be 1, i.e. 0xfe000000 .. 0xffffffff.

  // Ask for a page in a specific place.  Note that this uses Mach's
  // vm_allocate, not mmap(), as mmap() + MAP_FIXED will happily map
  // over an existing memory mapping, and there does not seem to be
  // a convenient (unix-only) API to query whether the memory region
  // is already taken.
  vm_address_t limit = 0xfeffffff;
  vm_address_t address = 0xfe000000;
  kern_return_t retcode;
  do retcode = vm_allocate(mach_task_self(), &address, pagesize, FALSE);
  while (retcode != KERN_SUCCESS && (address += pagesize) < limit);
  void *addr = (address < limit ? (void *) address : MAP_FAILED);
#elif __linux__ && __x86_64__ || __aarch64__
#if __x86_64__
  // Find a memory page we can allocate in the same 32-bit section.
  // JMP instruction doesn't have an 8-byte address version, and in
  // any case we don't want to use that long instruction sequence:
  // we'd have to use at least 10-12 bytes of the function prefix,
  // which frequently isn't location independent so we'd have to
  // parse and rewrite the code if we copied it.
  const unsigned long mask = 0xffffffff00000000;
#else //__aarch64__
  // Find a memory page close enough we can jump to using a B instruction.
  const unsigned long mask = 0xfffffffff8000000;
#endif
  unsigned long address = (unsigned long) target;
  unsigned long baseaddr = (address & mask);
  unsigned long freepage = address + 1;

  FILE *maps = fopen("/proc/self/maps", "r");
  if (maps)
  {
    while (! feof(maps))
    {
      char range[128];
      range[0] = 0;

      for (size_t i = 0; i < sizeof(range)-1; ++i)
      {
	int c = fgetc(maps);
	if (c == EOF || c == ' ' || c == '\n')
	  break;

	range[i] = c;
	range[i+1] = 0;
      }

      unsigned long low, high;
      if (sscanf(range, "%lx-%lx", &low, &high) != 2)
	continue;

      if ((low & mask) == baseaddr
	  && freepage >= low
	  && freepage < high)
	freepage = high;
    }

    fclose(maps);
  }

  if ((freepage & mask) != baseaddr)
  {
    ptr = 0;
    return IgHook::ErrAllocateTrampoline;
  }

  void *addr = mmap((void *) freepage, pagesize,
		    PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
#else
  // Just ask for a page.  Let system position it, so we don't unmap
  // or remap over address space accidentally.
  void *addr = mmap(0, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
  if (addr != MAP_FAILED)
  {
    unsigned int *page = (unsigned int *) addr;
    *page = pagesize;
    ptr = ++page;
    return IgHook::Success;
  }
  else
  {
    ptr = 0;
    return IgHook::ErrAllocateTrampoline;
  }
}

/** Release a trampoline previously created by #allocate(). */
static void
release(void *ptr)
{
  unsigned int *page = (unsigned int *) ptr;
  --page;
  munmap(page, *page);
}

/** Change memory protection for code at @a address.  Sets @a address
    to readable and executable, plus writable if @a writable is true.
    Call this function with @a writable = true before messing with
    existing code segments, and with @a writable = false after code
    has been edited.  */
static IgHook::Status
protect(void *address, bool writable, unsigned prologueSize)
{
  assert(sizeof(address) <= sizeof(unsigned long));

  void *originalAddress = address;
  int pagesize = getpagesize();
  address = (void *) (((unsigned long) address) & ~(pagesize-1));

  // If the function prologueSize is at the edge of
  // the page change protection also from next page.
  if (((pagesize - ((unsigned long)originalAddress - (unsigned long)address)))
        <= prologueSize)
    pagesize += prologueSize;
#if __APPLE__
  // (http://lists.apple.com/archives/Darwin-kernel/2005/Feb/msg00045.html)
  // The dynamic loader (dyld) loads pages into unmodifiable system-wide
  // shared map.  The first time we touch a page we need to make a
  // writable process-local copy of the page; on subsequent uses we
  // can just go ahead and flip the page writable.  We can't give
  // executable permissions on the page however.
  mach_port_t self = mach_task_self();
  vm_address_t vmaddr = (vm_address_t) address;
  vm_prot_t protection = VM_PROT_READ | VM_PROT_WRITE;
  kern_return_t retcode = vm_protect(self, vmaddr, pagesize, false, protection);;
  if (writable && retcode != KERN_SUCCESS)
  {
    protection = VM_PROT_READ | VM_PROT_COPY;
    retcode = vm_protect(self, vmaddr, pagesize, FALSE, protection);
  }
  if (retcode != KERN_SUCCESS)
  {
    igprof_debug("vm_protect(%p, %d, %d) failed: %d\n",
		 address, pagesize, protection, retcode);
    return IgHook::ErrMemoryProtection;
  }
#else
  int protection = PROT_READ | PROT_EXEC | (writable ? PROT_WRITE : 0);
  if (mprotect(address, pagesize, protection))
  {
    igprof_debug("mprotect(%p, %d, %d) failed: %d\n",
		 address, pagesize, protection, errno);
    return IgHook::ErrMemoryProtection;
  }
#endif

  return IgHook::Success;
}

/** Flush data and instruction caches.  Called when we are finished with
    modifying code in place to ensure the copies have made it to memory
    and processor will use the new versions.  */
static void
flush(void *address)
{
  msync(address, TRAMPOLINE_SIZE, MS_INVALIDATE);
}

/** Adjust @a ptr by @a n bytes.  Returns the modified @a ptr.  */
static void *
skip(void *&ptr, int n)
{ ptr = (unsigned char *) ptr + n; return ptr; }

//////////////////////////////////////////////////////////////////////
#define ELFM2_(n,t)             ELF ## n ## _ ## t
#define ELFM_(n, t)             ELFM2_(n,t)
#define ELFM(t)                 ELFM_(__ELF_NATIVE_CLASS,t)
#define ELF_R_SYM(x)            ELFM(R_SYM)(x)
#define ELF_ST_BIND(x)          ELFM(ST_BIND)(x)
#define ELF_ST_TYPE(x)          ELFM(ST_TYPE)(x)
#define ELF_ST_VISIBILITY(x)    ELFM(ST_VISIBILITY)(x)

/** Locate and map in the binary. */
static int
findAndMapBinary(dl_phdr_info *info,
                 char *&objname,
                 char *&objrname,
                 char *&freeobjname,
                 const char *&objimg,
                 off_t &size,
                 IgHookFindSym *match,
                 struct stat &st)
{
  int fd = -1;
  int ret = 0;
  void *addr;

  if (info->dlpi_name
      && *info->dlpi_name == '/'
      && (fd = open(info->dlpi_name, O_RDONLY)) >= 0)
  {
    objname = (char *) info->dlpi_name;
    ret = 1;
  }
  else
  {
    // Pick an address we'll know we'll be looking for.
    unsigned long address = 0;
    for (int j = 0; j < info->dlpi_phnum; ++j)
      if (info->dlpi_phdr[j].p_type == PT_LOAD)
        address = info->dlpi_addr + info->dlpi_phdr[j].p_vaddr;

    // Scan address map for a range around 'address'.
    // Read memory address map.
    FILE *maps = fopen("/proc/self/maps", "r");
    if (! maps || ferror(maps))
    {
      if (maps)
        fclose(maps);
      return -1;
    }

    // Scan address map for a range around 'address'.
    while (! feof(maps))
    {
      // Get mapping parameters.
      int c;
      char prot[5];
      long inode;
      unsigned long begin, end, offset, devmajor, devminor;
      if (fscanf(maps, "%lx-%lx %4s %lx %lx:%lx %ld",
                 &begin, &end, prot, &offset,
                 &devmajor, &devminor, &inode)
          != 7)
        break;

      // Skip if this range is not of interest.
      if (! (address >= begin && address < end))
      {
        while ((c = fgetc(maps)) != EOF && c != '\n')
          /* empty */;

        continue;
      }

      // Extract file name part (if any) after skipping space.
      while ((c = fgetc(maps)) != EOF && (c == ' ' || c == '\t'))
        /* empty */;

      size_t n = 0;
      size_t len = 0;
      while (c != EOF && c != '\n')
      {
        if (! len || n == len-1)
        {
          len = len * 2 + 1024;
          char *b = (char *) realloc(objname, len);
          if (! b)
          {
            fclose(maps);
            free(objname);
            objname = 0;
            return -1;
          }
          objname = b;
        }

        objname[n++] = c;
        c = fgetc(maps);
      }

      if (objname)
      {
        assert(len > 0);
        assert(n < len);
        objname[n++] = 0;
        fd = open(objname, O_RDONLY);
        freeobjname = objname;
        ret = 1;
      }

      break;
    }

    fclose(maps);
  }

  if (fd >= 0)
  {
    if (fstat(fd, &st) == 0)
    {
      if (! match->st || (match->st->st_dev == st.st_dev
                          && match->st->st_ino == st.st_ino))
      {
        size = st.st_size;
        if ((addr = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0)) != MAP_FAILED)
          objimg = (const char *) addr;
      }
      else
      {
        igprof_debug("%s candidate object %s is not a match for %s,"
                     " dev:ino mismatch %ld:%ld vs. %ld:%ld, skipping\n",
                     match->sym, objname, match->lib,
                     (long) st.st_dev, (long) st.st_ino,
                     (long) match->st->st_dev, (long) match->st->st_ino);
        objname = 0;
        ret = 0;
      }
    }

    close(fd);
  }

  if (objname)
    objrname = realpath(objname, 0);

  assert(! objname || ret > 0);
  assert(! objimg || objname);
  assert(! objrname || objname);
  return ret;
}

/** Try locating a symbol in a table. */
void *
findSymInTable(const char *symbol,
               const char *object,
               const char *section,
               const ElfW(Shdr) *shdr,
               const ElfW(Sym) *syms,
               const char *strs,
               const ElfW(Word) *xsndx,
               uint32_t nsyms,
               ElfW(Addr) vmaslide,
               ElfW(Addr) vmabase)
{
  if (! syms || ! strs)
    return 0;

  for (uint32_t i = 0; i < nsyms; ++i)
  {
    const ElfW(Shdr) *sect = 0;
    switch (syms[i].st_shndx)
    {
    case SHN_UNDEF:
    case SHN_BEFORE:
    case SHN_AFTER:
    case SHN_ABS:
    case SHN_COMMON:
      break;

    case SHN_XINDEX:
      sect = shdr && xsndx ? shdr + xsndx[i] : 0;
      break;

    default:
      sect = shdr ? shdr + syms[i].st_shndx : 0;
      break;
    }

    unsigned type = ELF_ST_TYPE(syms[i].st_info);
    ElfW(Addr) addr = vmaslide + syms[i].st_value;
    ElfW(Addr) offset = vmaslide + syms[i].st_value - vmabase;
    if (syms[i].st_shndx == SHN_UNDEF)
      addr = offset = 0;
    else if (! addr
             || type == STT_TLS
             || (sect && ! (sect->sh_flags & SHF_ALLOC)))
      offset = syms[i].st_value;

    if (addr && ! strcmp(symbol, strs + syms[i].st_name))
    {
      igprof_debug("%s found at address 0x%lx, offset 0x%lx in %s section of %s\n",
                   symbol, addr, offset, section, object ? object : "(unknown)");
      return (void *) addr;
    }
  }

  return 0;
}



/** Try locating a symbol by name in a binary. */
static int
maybeFindSymbolInBinary(dl_phdr_info *info, size_t /* size */, void *ptr)
{
  struct stat st;
  IgHookFindSym *match = (IgHookFindSym *) ptr;
  if (match->sym)
    return 0;

  bool vmafound = false;
  ElfW(Addr) vmaslide = info->dlpi_addr;
  ElfW(Addr) vmabase = 0;
  const ElfW(Dyn) *dyn = 0;
  char *objname = 0;
  char *objrname = 0;
  char *freeobjname = 0;
  const char *objimg = 0;
  off_t objsize = 0;
  int ret = findAndMapBinary(info, objname, objrname, freeobjname, objimg, objsize, match, st);

  if (ret <= 0)
  {
    if (freeobjname)
      free(freeobjname);
    return 0;
  }

  for (int i = 0; i < info->dlpi_phnum; ++i)
  {
    const ElfW(Phdr) &phdr = info->dlpi_phdr[i];
    ElfW(Addr) addr = vmaslide + phdr.p_vaddr;
    switch (phdr.p_type)
    {
    case PT_DYNAMIC:
      dyn = (const ElfW(Dyn) *) addr;
      break;

    case PT_PHDR:
    case PT_LOAD:
      if (! vmafound)
      {
        vmabase = addr - phdr.p_offset;
        vmafound = true;
      }
      break;
    }
  }

  // Maybe locate data for interpreting dynamic symbol tables.
  if (vmafound && dyn)
  {
    const uint32_t *oldhash = 0;
    const uint32_t *gnuhash = 0;
    const ElfW(Sym) *dsyms = 0;
    const char *dstrs = 0;
    uint32_t ndsyms = 0;
    ElfW(Addr) slide = (!strcmp(objname, "[vdso]") ? vmaslide : 0);

    for (int i = 0; dyn[i].d_tag != DT_NULL; ++i)
    {
      switch (dyn[i].d_tag)
      {
      case DT_SYMTAB:
        dsyms = (const ElfW(Sym) *) (dyn[i].d_un.d_ptr + slide);
        break;

      case DT_STRTAB:
        dstrs = (const char *) (dyn[i].d_un.d_ptr + slide);
        break;

      case DT_GNU_HASH:
        gnuhash = (const uint32_t *) (dyn[i].d_un.d_ptr + slide);
        break;

      case DT_HASH:
        oldhash = (const uint32_t *) (dyn[i].d_un.d_ptr + slide);
        break;
      }
    }

    // Figure out from hash tables how many dynamic symbols there are.
    if (dsyms && dstrs && gnuhash)
    {
      bool ok = true;
      uint32_t nbuckets = *gnuhash++;
      uint32_t minidx = *gnuhash++;
      uint32_t maskwords = *gnuhash++;
      const uint32_t *buckets =
          gnuhash + 1 + maskwords * (sizeof(ElfW(Addr)) / sizeof(uint32_t));
      const uint32_t *chains = buckets + nbuckets - minidx;
      uint32_t maxidx = 0xffffffff;

      for (uint32_t i = 0; i < nbuckets; ++i)
        if (buckets[i] == 0)
          ;
        else if (buckets[i] < minidx)
        {
          ok = false;
          break;
        }
        else if (maxidx == 0xffffffff || buckets[i] > maxidx)
          maxidx = buckets[i];

      if (ok && maxidx != 0xffffffff)
      {
        while ((chains[maxidx] & 1) == 0)
          ++maxidx;

        ndsyms = maxidx + 1;
      }
    }

    if (ndsyms == 0 && dsyms && dstrs && oldhash)
      ndsyms = oldhash[1];

    // Look for the symbol.
    match->sym = findSymInTable(match->fn, objname, "dynsym",
                                0, dsyms, dstrs, 0,
                                ndsyms, vmaslide, vmabase);
  }

  // Maybe locate data for interpreting the regular symbol tables.
  if (! match->sym && objimg)
  {
    const ElfW(Ehdr) *ehdr = (const ElfW(Ehdr) *) objimg;
    const ElfW(Shdr) *shdr = (const ElfW(Shdr) *) (objimg + ehdr->e_shoff);
    const ElfW(Sym) *syms = 0;
    const ElfW(Word) *xsndx = 0;
    uint32_t nsyms = 0;
    const char *strs = 0;

    for (unsigned i = 0, e = ehdr->e_shnum; i != e && ! match->sym; ++i)
    {
      if (shdr[i].sh_type == SHT_SYMTAB)
      {
        strs = objimg + shdr[shdr[i].sh_link].sh_offset;
        syms = (const ElfW(Sym) *) (objimg + shdr[i].sh_offset);
        nsyms = shdr[i].sh_size / shdr[i].sh_entsize;

        // Look for SHT_SYMTAB_SHNDX; should not really be present in DSOs.
        for (unsigned j = i+1; j != e && ! xsndx; ++j)
          if (shdr[j].sh_type == SHT_SYMTAB_SHNDX && shdr[j].sh_link == i)
            xsndx = (const ElfW(Word) *) (objimg + shdr[j].sh_offset);
        for (unsigned j = 0; j != i && ! xsndx; ++j)
          if (shdr[j].sh_type == SHT_SYMTAB_SHNDX && shdr[j].sh_link == i)
            xsndx = (const ElfW(Word) *) (objimg + shdr[j].sh_offset);

        match->sym = findSymInTable(match->fn, objname, "symtab",
                                    shdr, syms, strs, xsndx,
                                    nsyms, vmaslide, vmabase);
      }
    }
  }

  // If we mapped in the object, release it now.
  if (objimg)
    munmap((void *) objimg, objsize);

  // Free object name if we pulled it from scanning /proc/self/maps.
  if (freeobjname)
    free(freeobjname);

  // Release true path if we have one.
  if (objrname)
    free(objrname);

  return 0;
}

/** Find a symbol by name, if it couldn't be found dynamically.

    This loads the ELF object and parses the symbol tables to locate
    the address for the requested symbol. */
static IgHook::Status
findsym(const char *fn, const char *v, const char *lib,
        void *&sym, IgHook::Status errcode)
{
  // We don't do versions (at least not yet).
  if (v)
    return errcode;

  IgHookFindSym info = { 0, fn, lib, 0 };
  struct stat st;

  // If a specific library was requested, speed things up by statting it.
  if (lib)
  {
    if (stat(lib, &st) != 0)
    {
      igprof_debug("findsym: no such library '%s' for '%s' (errno %d)\n",
                   lib, fn, errno);
      return IgHook::ErrSymbolNotFoundInLibrary;
    }
    else
      info.st = &st;
  }

  // Now look for the symbol. We don't really care about symbol lookup
  // order here since we can only come here if dynamic lookup already failed.
  dl_iterate_phdr(&maybeFindSymbolInBinary, &info);

  // If we found the symbol, return it with success, otherwise return failure.
  if (info.sym)
  {
    sym = info.sym;
    return IgHook::Success;
  }
  else
  {
    igprof_debug("findsym('%s', '%s', %d): symbol not found\n",
                 fn, lib, (int) errcode);
    return errcode;
  }
}

/** Find @a fn and assign it's address into @a sym.  If @a lib is
    non-null, the library is dynamically loaded and looked up in that
    library.  Otherwise looks for the symbol in already loaded
    libraries, or failing that, in the main program itself.  On
    failure returns error code, otherwise sets @a sym and returns
    success. */
static IgHook::Status
lookup(const char *fn, const char *v, const char *lib, void *&sym)
{
  sym = 0;

  if (lib)
  {
    void *handle = dlopen(lib, RTLD_LAZY | RTLD_GLOBAL);
    if (! handle)
    {
      igprof_debug("dlopen('%s'): %s\n", lib, dlerror());
      return IgHook::ErrLibraryNotFound;
    }

    sym = v ? dlvsym(handle, fn, v) : dlsym(handle, fn);
    if (! sym)
    {
      igprof_debug("dlsym('%s', '%s'): %s\n", lib, fn, dlerror());
      return findsym(fn, v, lib, sym, IgHook::ErrSymbolNotFoundInLibrary);
    }
  }
  else
  {
    void *program = dlopen(0, RTLD_LAZY | RTLD_GLOBAL);
    sym = v ? dlvsym(program, fn, v) : dlsym(program, fn);
    dlclose(program);
    if (! sym) sym = v ? dlvsym(program, fn, v) : dlsym(RTLD_NEXT, fn);

#if __arm__
    if (sym)
    {
      unsigned *insns = (unsigned *)sym;
      /* If the program is using libpthread. Fork and system functions got to be
         hooked in libc.so.6 instead
         dlsym can return address which points to .PLT section. If try to find
         the real location with RTLD_NEXT option */
      if (*insns == 0xeaffcbed || *insns == 0xeaffd0a5)
      {
        igprof_debug("Function %s hooked in libc.so.6 instead RTLD_NEXT\n",fn);
        void *handle = dlopen("libc.so.6", RTLD_LAZY | RTLD_GLOBAL);
        sym = dlsym(handle, fn);
      }
      else if ((insns[0] & 0xfffff000) == 0xe28fc000
            && (insns[1] & 0xfffff000) == 0xe28cc000
            && (insns[2] & 0xfffff000) == 0xe5bcf000)
      {
        igprof_debug("sym pointing to .plt section, trying dlsym(RTLD_NEXT,fn)\n");
        sym = dlsym(RTLD_NEXT, fn);
      }
    }
#endif

    if (! sym)
    {
      igprof_debug("dlsym(self, '%s'): %s\n", fn, dlerror());
      return findsym(fn, v, 0, sym, IgHook::ErrSymbolNotFoundInSelf);
    }
  }

  return IgHook::Success;
}

/*
 * Function to help evaluate instruction lenght. Parse function calls this when
 * modRM byte is part of instruction. Returns lenght of instruction.
 */
int evalModRM(unsigned char byte, modRMByte &modRM)
{
  modRM.encoded = byte;
  //mod == 00 and rm == 5	opcode, modRM, rip + 32bit
  //mod == 00                   opcode, modRM,(SIB)
  //mod == 01                   opcode,modRM,(SIB),1 byte immediate
  //mod == 10                   opcode,modRM,(SIB),4 byte immeadiate
  //mod == 11                   opcode,modRM
  if (modRM.bits.mod == 0 && modRM.bits.rm == 5)
    return 6;  //caller handles patching
  else if (modRM.bits.mod == 0)
    return (modRM.bits.rm) != 4 ? 2 : 3;	//check if SIB byte is needed
  else if (modRM.bits.mod == 1)
    return (modRM.bits.rm) != 4 ? 3 : 4;	//check if SIB byte is needed
  else if (modRM.bits.mod == 2)
    return (modRM.bits.rm) != 4 ? 6 : 7;	//check if SIB byte is needed

  return 2;
}

/** Parse function prologue at @a address.  Returns the number of
    instructions understood that need to be moved out to insert a jump
    to the trampoline, or -1 if a sufficiently long safe sequence was
    not found.

    Other prefixes but rex prefixes(4*), opcodes 0F group, 6C-6F, 8C, 8E, 98-9F,
    A0-A7, AA-AF, C2-C5, D6-DF, E0-E3,EC-EF,F0-FD are not supported.
    Group FF is partly supported
*/

static int
parse(const char *func, void *address, unsigned *patches)
{
  int n = 0;
#if __i386__
  unsigned char *insns = (unsigned char *) address;
  if (insns[0] == 0xe9)
  {
    igprof_debug("%s (%p): hook trampoline already installed, ignoring\n",
		 func, address);
    return -1;
  }

  while (n < 5)
  {
    if (insns[0] >= 0x50 && insns[0] <= 0x57) /* push %e*x */
      ++n, ++insns;

    else if (insns[0] == 0x89 && insns[1] == 0xe5) /* mov %esp, %ebp */
      n += 2, insns += 2;

    else if (insns[0] == 0x89 && insns[1] == 0xda) /* mov %ebx, %edx */
      n += 2, insns += 2;

    else if (insns[0] == 0x83 && insns[1] == 0xec) /* sub $0x*, %esp */
      n += 3, insns += 3;

    else if (insns[0] == 0x81 && insns[1] == 0xec) /* sub $0x*, %esp (32-bit) */
      n += 6, insns += 6;

    else if (insns[0] == 0x8b && insns[2] == 0x24) /* mov 0x4(%esp,1),%e*x */
      n += 4, insns += 4;

    else if (insns[0] == 0x8d && insns[1] == 0x55) /* lea $0x*(%ebp),%edx */
      n += 3, insns += 3;

    else if (insns[0] >= 0xb8 && insns[0] <= 0xbf) /* mov $0xNN,%e*x */
      n += 5, insns += 5;

    else if (insns[0] == 0xff && insns[1] == 0x25) /* jmp *addr */
      n += 6, insns += 6;

    else if (insns[0] == 0x65 && insns[1] == 0x83 && insns[2] == 0x3d)
      n += 8, insns += 8;                          /* cmpl $0x*,%gs:0x* */

    else if (insns[0] == 0xe8) /* call +offset (32-bit) */
      *patches++ = n+1, n += 5, insns += 5;

    else
    {
      igprof_debug("%s (%p) + 0x%x: unrecognised prologue (found 0x%x)\n",
		   func, address, insns - (unsigned char *) address, *insns);
      return -1;
    }
  }
#elif __x86_64__
  int temp = 0;
  unsigned char *insns = (unsigned char *) address;
  modRMByte modRM;

  if (insns[0] == 0xe9)
  {
    unsigned long target = (unsigned long) insns + *(int *)(insns+1) + 5;
    if ((target & 0xfff) == 0x004)
    {
      igprof_debug("%s (%p): hook trampoline already installed, ignoring\n",
		   func, address);
      return -1;
    }
    else
      igprof_debug("%s (%p): jump instruction found, but not a hook target\n",
		   func, address);
  }

  while (n < 5)
  {
    if (insns[0] >= 0x40 && insns[0] <= 0x4f)
    {
      insns += 1;
      n += 1;
    }

    //one byte instructions
    if ((insns[0] >= 0x50 && insns[0] <= 0x5f)
       || (insns[0] >= 0x90 && insns[0] <= 0x97))
      ++insns, ++n;

    //opcode + one byte
    else if ((insns[0] >= 0xb0 && insns[0] <= 0xb7)
      	     || (insns[0] >= 0xd0 && insns[0] <= 0xd3)
    	     || (insns[0] >= 0xe4 && insns[0] <= 0xe7)
    	     || insns[0] == 0x04 || insns[0] == 0x14
    	     || insns[0] == 0x24 || insns[0] == 0x34
    	     || insns[0] == 0x0c || insns[0] == 0x1c
    	     || insns[0] == 0x2c || insns[0] == 0x3c
    	     || insns[0] == 0xa1 || insns[0] == 0xa8
    	     || insns[0] == 0x6a)
      insns += 2, n += 2;

    //opcode + 4 bytes
    else if ((insns[0] >= 0xb8 && insns[0] <= 0xbf)
    	     || insns[0] == 0x05 || insns[0] == 0x15
    	     || insns[0] == 0x25 || insns[0] == 0x35
    	     || insns[0] == 0x0d || insns[0] == 0x1d
    	     || insns[0] == 0x2d || insns[0] == 0x3d
    	     || insns[0] == 0xa9 || insns[0] == 0x68)
      insns += 5, n += 5;

    //jmp /call 4bytes offset
    else if (insns[0] == 0xe8 || insns[0] == 0xe9)
      *patches++ = (n+0x5)*0x100 + n+1, n += 5, insns += 5;

    // je <32bit offset>
    else if (insns[0] == 0x0f && insns[1] == 0x84)
      *patches++ = (n+0x6)*0x100 + n+2, n += 6, insns += 6;

    // opcode + modRM (no immediate)
    else if ((insns[0] <= 0x03)
             || (insns[0] >= 0x08 && insns[0] <= 0x0b)
             || (insns[0] >= 0x10 && insns[0] <= 0x13)
             || (insns[0] >= 0x18 && insns[0] <= 0x1b)
             || (insns[0] >= 0x20 && insns[0] <= 0x23)
             || (insns[0] >= 0x28 && insns[0] <= 0x2b)
             || (insns[0] >= 0x30 && insns[0] <= 0x33)
             || (insns[0] >= 0x38 && insns[0] <= 0x3b)
             || (insns[0] >= 0x84 && insns[0] <= 0x8b)
             || insns[0] == 0x8d || insns[0] == 0x63
             || insns[0] == 0xc0 || insns[0] == 0xc1)
    {
      temp = evalModRM(insns[1], modRM);
      if (temp == 6 && modRM.bits.mod == 0) //opcode, modRM, rip + 32bit
      	*patches++ = (n+0x6)*0x100 + n+2, n += 6, insns += 6;
      else	//opcode, modRM,(SIB)
        insns += temp, n += temp;
    }
    //opcode, modRM,(sib),1 or 4 byte immediate
    else if ((insns[0] >= 0x80 && insns[0] <= 0x83)
    	     || insns[0] == 0x69 || insns[0] == 0x6b
    	     || insns[0] == 0xc0 || insns[0] == 0xc1
	     || insns[0] == 0xd0 || insns[0] == 0xd1
	     || insns[0] == 0xfe || insns[0] == 0xc6
	     || insns[0] == 0xc7 || insns[0] == 0xf6
	     || insns[0] == 0xf7)
    {
      if (insns[0] == 0xc6 || insns[0] == 0xc7) //opcode groups
      {
        modRM.encoded = insns[1];
        if(modRM.bits.reg != 0)
          return -1;
      }
      temp = evalModRM(insns[1], modRM);

      if (temp == 6 && modRM.bits.mod == 0)	//rip + 32bit
      {
        if (insns[0] == 0x81 || insns[0] == 0x69	//4byte immediate
	    || insns[0] == 0xc7)
          *patches++ = (n+0xa)*0x100 + n+2, n += 10, insns += 10;
        else  //one byte immediate
          *patches++ = (n+0x7)*0x100 + n+2, n += 7, insns +=7;
      }
      else
      {
        if (insns[0] == 0x81 || insns[0] == 0x69
            || insns[0] == 0xc7)
          n += (temp + 4), insns += (temp + 4);
        else
          n += (temp + 1), insns += (temp + 1);
      }
    }
    // f6 and f7 group
    else if (insns[0] == 0xf6 || insns[0] == 0xf7)
    {
      temp = evalModRM(insns[1], modRM);
      if (modRM.bits.reg == 0 || modRM.bits.reg == 1) //instruction needs immediate value
      {
        if (temp == 6 && modRM.bits.mod == 0)
        {
          if (insns[0] == 0xf6)  //one byte immediate
            *patches++ = (n+0x7)*0x100 + n+2, n += 7, insns += 7;
          else  //4 byte immediate
            *patches++ = (n+0xa)*0x100 + n+2, n += 10, insns += 10;
        }
      }
      else if (temp == 6 && modRM.bits.mod == 0)	//rip + 32bit
      	*patches++ = (n+0x6)*0x100 + n+2, n += 6, insns += 6;
      else
        n += temp, insns += temp;
    }
    //0xff group
    else if (insns[0] == 0xff)
    {
      temp = evalModRM(insns[1], modRM);
      if (modRM.bits.reg == 3 || modRM.bits.reg == 5)
        return -1;
      else if (temp == 6 && modRM.bits.mod == 0)	//rip + 32bit
      	*patches++ = (n+0x6)*0x100 + n+2, n += 6, insns += 6;
      else
        n += temp, insns += temp;
    }
    //syscall
    else if (insns[0] == 0xf && insns[1] == 0x5)
      n += 2, insns += 2;


    else if (insns[0] == 0xf3 && insns[1] == 0xc3)
      n +=5, insns += 5;

    else
    {
      igprof_debug("%s (%p) + 0x%x: unrecognised prologue (found 0x%x 0x%x 0x%x 0x%x)\n",
		   func, address, insns - (unsigned char *) address,
		   insns[0], insns[1], insns[2], insns[3]);
      return -1;
    }
    temp = 0;
  }
#elif __ppc__
  // FIXME: check for various branch-relative etc. instructions
  assert(sizeof (unsigned int) == 4);
  unsigned int *insns = (unsigned int *) address;
  unsigned int instr = *insns;
  if ((instr & 0xfc1fffff) == 0x7c0903a6) // check it's not mfctr
  {
    igprof_debug("%s (%p): mfctr can't be instrumented\n", func, address);
    return -1;
  }

  n = 4;
#elif __arm__
  unsigned  *insns = (unsigned *) address;
  if (insns[0] == 0xe51ff004)
  {
    igprof_debug("%s (%p): hook trampoline already installed, ignoring\n",
                 func, address);
    return -1;
  }
  while (n < 8)
  {
    if ((insns[0] & 0xffff0000) == 0xe59f0000       //ldr r*, [PC + #***]
     || (insns[0] & 0xffff0000) == 0xe51f0000       //ldr r*, [PC - #***]
     || (insns[0] == 0xe79fc00c))                    //ldr ip, [PC, ip]
    {
      if (n >= 4 && (*patches == 0x0))
      {
        *patches++ = 0xffffffff;
      }
      n += 4, *patches++ = *insns, insns++;
    }
    else if ((insns[0] & 0xffff0000) == 0xe92d0000) // push {reglist}
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xffff0000) == 0xe1a00000) // mov rd, rn
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xffff0000) == 0xe3a00000) // mov rd, #imm
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xfff00000) == 0xe3100000) // tst rx, #imm
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xfff00000) == 0xe3500000) // cmp rx, #imm
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xff000000) == 0xee000000) // mcr
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xffff0000) == 0xe52d0000) // push {reglist}
    {
      n += 4, insns++;
    }
    else if ((insns[0] & 0xffff0000) == 0xe49d0000) // pop {reglist}
    {
      n += 4, insns++;
    }
    else if (  insns[0] == 0xe1812000   // orr r2, r1, r0
            || insns[0] == 0xe2504000   // subs r4, r0, #0
            || insns[0] == 0xe2403020)  // sub r3, r0, #0
    {
      n += 4, insns++;
    }
    else
    {
      igprof_debug("%s (%p) + 0x%x: unrecognised prologue (found 0x%x)\n",
		   func, address, insns - (unsigned *) address,
		   insns[0]);
      return -1;
    }
  }
#elif __aarch64__
  uint32_t *insns = (uint32_t *) address;

  if (insns[0] == ENCODE_LDR(TEMP_REG, 8) // LDR X16, .+8
      && insns[1] == ENCODE_BR(TEMP_REG)) // BR X16
  {
    igprof_debug("%s (%p): hook trampoline already installed, ignoring\n",
                 func, address);
    return -1;
  }
  else if ((insns[0] & 0xfc000000) == 0x14000000) // B instruction
  {
    if ((((insns[0] << 2) + (uint64_t)insns) & 0xfff) == 0x004)
    {
      igprof_debug("%s (%p): hook trampoline already installed, ignoring\n",
                   func, address);
      return -1;
    }
    else
    {
      igprof_debug("%s (%p): branch instruction found, but not a hook target\n",
                   func, address);
    }
  }

  while (n < TRAMPOLINE_JUMP)
  {
    // Each patch entry contains one's complement of the offset (in bytes)
    // to an instruction that needs to be patched because of PC-relative
    // addressing. The patch list is terminated by 0 (not in one's complement).
    // This allows a distinction between offset 0 and the end of the patch
    // list.
    if ((insns[0] & 0x1f000000) == 0x10000000 // ADR(P) instruction
        || (insns[0] & 0xff000010) == 0x54000000 // B.cond instruction
        || (insns[0] & 0x5c000000) == 0x14000000 // B(L), CB(N)Z or TB(N)Z
        || ((insns[0] & 0x3b000000) == 0x18000000 // LDR(SW) instruction
            && (insns[0] & 0xc0000000) != 0xc0000000))
    {
      *patches++ = ~n;
    }

    n += 4;
    ++insns;
  }
#endif

  *patches = 0;
  return n;
}

/** Insert escape jump from address @a from to address @a to.  Caller
    must ensure offset for @a to is suitable for a single branch
    instruction with limited number of bits for immediate argument on
    architectures where only a single instruction is replaced in the
    prologue.  */
static int
redirect(void *&from, void *to, IgHook::JumpDirection direction UNUSED)
{
#if __i386__
  // NB: jump offsets are calculated from *after* the jump instruction
  unsigned char *start = (unsigned char *) from;
  unsigned char *insns = (unsigned char *) from;
  unsigned long diff = (unsigned long) to - ((unsigned long) from + 5);
  *insns++ = 0xe9;
  *insns++ = diff & 0xff;
  *insns++ = (diff >> 8) & 0xff;
  *insns++ = (diff >> 16) & 0xff;
  *insns++ = (diff >> 24) & 0xff;
  from = insns;
  return insns - start;

#elif __x86_64__
  // This is essentially similar to i386 mode however there is no
  // "large model" jmp instruction.  We have to load the address
  // to a register and then jump indirectly via the register.
  // The ABI reserves r11 register for scratch pad use like this.
  unsigned char *start = (unsigned char *) from;
  unsigned char *insns = (unsigned char *) from;
  unsigned long addrloc;
  unsigned long diff;

  switch (direction)
  {
  case IgHook::JumpToTrampoline:
    // The jump to the trampoline is like i386, with 32-bit offset.
    diff = (unsigned long) to - ((unsigned long) from + 5);
    *insns++ = 0xe9;
    *insns++ = diff & 0xff;
    *insns++ = (diff >> 8) & 0xff;
    *insns++ = (diff >> 16) & 0xff;
    *insns++ = (diff >> 24) & 0xff;
    break;

  case IgHook::JumpFromTrampoline:
    addrloc = ((unsigned long) insns + 9 + 15) & 0xfffffffffffffff0;
    *insns++ = 0x4c; // 0-6: movq N(%rip),%r11
    *insns++ = 0x8b;
    *insns++ = 0x1d;
    *insns++ = addrloc - (unsigned long) start - 7; // N
    *insns++ = 0x00;
    *insns++ = 0x00;
    *insns++ = 0x00;
    *insns++ = 0x41; // 7-9: jmp *%r11
    *insns++ = 0xff;
    *insns++ = 0xe3;
    while ((unsigned long) insns < addrloc)
      *insns++ = 0x90; // (gap): nop

    *(unsigned long *)insns = (unsigned long) to;
    insns += 8;
    break;
  }
  from = insns;
  return insns - start;

#elif __ppc__
  // The low six bits are "ba" instruction (opcode 18 = 0x12),
  // then immediate address with the low two bits stripped off,
  // and top two bits are "01" (no link, absolute).  This only
  // works if the address is appropriate.  The 24-bit immediate
  // address is sign extended to 32 bits, so either it must be
  // in the low 23-bit address space, or in the high area.
  unsigned int *start = (unsigned int *) from;
  unsigned int *insns = (unsigned int *) from;

  assert(sizeof (unsigned int) == 4);
  assert(! ((unsigned int) to & 0x3));
  // *insns++ = 0x40000012 | (((unsigned int) to >> 2) << 5); // ba to
  *insns++ = 0x48000002 | ((unsigned int) to & 0x3ffffff);
  from = insns;
  return (insns - start) * 4;
#elif __arm__
  unsigned *start = (unsigned *) from;
  unsigned *insns = (unsigned *) from;
  *insns++ = 0xe51ff004;  // LDR PC, [PC,- 4]
  *insns++ = (unsigned) to;  // address to jump
  from = insns;
  return (insns - start) * 4;
#elif __aarch64__
  uint32_t *start = (uint32_t *) from;
  uint32_t *insns = (uint32_t *) from;
  int64_t diff = (uint64_t)to - (uint64_t)from;

  switch(direction)
  {
  // short jump
  case IgHook::JumpToTrampoline:
    assert(diff >= -(1 << 27) && diff < (1 << 27));
    *insns++ = ENCODE_B(diff);
    break;

  // long jump
  case IgHook::JumpFromTrampoline:
    *insns++ = ENCODE_LDR(TEMP_REG, 8);  // LDR X16, .+8
    *insns++ = ENCODE_BR(TEMP_REG);  // BR X16
    *(uint64_t *)insns = (uint64_t)to;  // address to jump to
    insns += 2;
    break;
  }

  from = insns;
  return (insns - start) * 4; // each instruction is 32 bits wide
#endif
}

/** Insert into address @a from the first part of a jump to address @a
    to.  Th instruction sequence can be longer than that allowed by
    #redirect(), and thus @a to can be an arbitrary address.  The
    function returns the number of bytes inserted.  It also updates @a
    from to the next address after the jump.  The caller can then
    insert other instructions to be executed just before the jump, and
    should then call #postreentry() to insert the actual jump
    code.  */
static int
prereentry(void *&from, void *to)
{
#if __ppc__
  // Set ctr using r0 as a temporary register.  The assumption here
  // is that this instruction sequence comes as immediate target of
  // a call in the re-entry part of the trampoline, meaning that we
  // are allowed to trash r0 (it's a volatile register so caller
  // must have saved it).  The instruction copied from the original
  // prologue comes after this one and must not trash ctr (parse()
  // ensures that) but may trash r0 after us.
  assert(sizeof(to) == sizeof(unsigned int));
  assert(sizeof(unsigned int) == 4);
  unsigned int *start = (unsigned int *) from;
  unsigned int *insns = (unsigned int *) from;
  *insns++ = 0x3c000000 | (((unsigned int) to & 0xffff0000) >> 16); // lis r0,addrhi
  *insns++ = 0x60000000 | (((unsigned int) to & 0x0000ffff) >> 0);  // ori r0,r0,addrlo
  *insns++ = 0x7c0903a6; // mtctr r0
  from = insns;
  return (insns - start) * 4;
#else
  // nothing to do
  (void) from;
  (void) to;
  return 0;
#endif
}

/** Insert into address @a from the final jump to address @a to.  */
static int
postreentry(void *&from, void *to)
{
#if __i386__ || __x86_64__ || __arm__ || __aarch64__
  // Real jump
  return redirect(from, to, IgHook::JumpFromTrampoline);
#elif __ppc__
  // ctr was set in prereentry(), jump into it
  assert(sizeof(unsigned int) == 4);
  unsigned int *start = (unsigned int *) from;
  unsigned int *insns = (unsigned int *) from;
  *insns++ = 0x4e800420; // bctr
  from = insns;
  return (insns - start) * 4;
#endif
}

/** Prepare a hook trampoline into @a address.  The first part of the
    trampoline is an unconditional jump instruction (not a call!) into
    the @a replacement function.  The second part is a copy of @a
    prologue bytes of preamble in the original function @a old,
    patched for PC-relative addressing according to @a patches,
    followed by another unconditional jump to the rest of the original
    function.  If @a chain is non-null, it will be set to point to
    this second part of the trampoline so @a replacement can call the
    uninstrumented original function.  */
static void
prepare(void *address,
	void *replacement, void **chain,
	void *old, int prologue, unsigned *patches UNUSED)
{
  // First part: unconditional jump to replacement
  prereentry(address, replacement);
  postreentry(address, replacement);

  // Second part: old function prologue + jump to post-prolugue code
  if (chain) *chain = address;
  prereentry(address, ((unsigned char *) old) + prologue);
#if __x86_64__ || __aarch64__
  void *start = address;
#endif
  memcpy(address, old, prologue);

#if __i386__
  // Patch i386 relative 'call' instructions found in prologue.
  // In practice there can be at most one patch since it's five
  // byte <0xe8, nn, nn, nn, nn> instruction. So if there is one
  // it must be the last instruction. Convert it to a push + jump
  // <0xff 0x35 nn nn nn nn> <0xe9 nn nn nn nn> to simulate call
  // coming from the original site - this is important in case
  // it's PIC __i686.get_pc_thunk.* call. Note both call and
  // jump instructions are relative to the address immediately
  // after the intruction. Obviously if we apply a patch, the
  // postreentry() created below will not be used.
  if (patches && *patches)
  {
    assert((unsigned) prologue == *patches + 4);

    // Compute addresses: original destination, where to patch.
    unsigned char *insns = (unsigned char *) address + *patches - 1;
    unsigned long retaddr = (unsigned long) old + *patches + 4;
    unsigned long offset = * (unsigned *) ((unsigned char *) old + *patches);
    unsigned long dest = retaddr + offset;

    // push <old-return-address>
    *insns++ = 0x68;
    *insns++ = retaddr & 0xff;
    *insns++ = (retaddr >> 8) & 0xff;
    *insns++ = (retaddr >> 16) & 0xff;
    *insns++ = (retaddr >> 24) & 0xff;

    // jmpq <delta-to-original-destination>
    unsigned long diff = dest - ((unsigned long) insns + 5);
    *insns++ = 0xe9;
    *insns++ = diff & 0xff;
    *insns++ = (diff >> 8) & 0xff;
    *insns++ = (diff >> 16) & 0xff;
    *insns++ = (diff >> 24) & 0xff;
  }
  else
  {
    skip(address, prologue);
    skip(old, prologue);
    postreentry(address, old);
  }
#elif __aarch64__
  skip(address, prologue);
  skip(old, prologue);
  // Use a short jump when jumping from the trampoline to the instrumented
  // function. IgHook::JumpToTrampoline is used in order to generate a short
  // jump, even if the jump is away from the trampoline.
  redirect(address, old, IgHook::JumpToTrampoline);
#else
  skip(address, prologue);
  skip(old, prologue);
  postreentry(address, old);
#endif

#if __x86_64__
  // Patch up PC-relative addresses. The patch values are of formed
  // Y << 8 + X, where X is offset into the instruction series
  // and Y is the delta to subtract: the instruction series length.
  for ( ; patches && *patches; ++patches)
  {
    unsigned patch = *patches;
    unsigned offset = patch & 0xff;
    unsigned delta = patch >> 8;
    *((unsigned *)((unsigned char *)start + offset))
      += (unsigned char *)old - (unsigned char *)start - delta;
  }
#elif __arm__
  // Patch position independent instruction to use PC + 8 location. Where the
  // content of the pointed location is copied.
  if (patches && *patches)
  {
    unsigned *insns = (unsigned *)address;
    for ( ; *patches && patches ; ++patches, ++insns)
    {
      if (*patches != 0xffffffff)
      {
        insns[-4] = (insns[-4] & 0x0000f000) + 0xe59f0008;
        if ((*patches & 0xffff0000) == 0xe59f0000)
        {
          memcpy(insns, (void *)((unsigned)old + (*patches & 0x00000fff)), 4);
        }
        else if ((*patches & 0xffff0000) == 0xe51f0000)
        {
          memcpy(insns, (void *)((unsigned)old - (*patches & 0x00000fff)), 4);
        }
        else if (*patches == 0xe79fc00c)
        {
          memcpy(insns, (void *)((unsigned)old + insns[-1]), 4);
        }
        skip(old,4);
      }
    }
  }
#elif __aarch64__
  // Patch PC-relative instructions
  if (patches)
  {
    uint32_t *patch_insns = (uint32_t *)address;
    uint8_t *old_prologue_start = (uint8_t *)old - prologue;

    for( ; *patches; ++patches)
    {
      unsigned int offset = ~*patches;
      uint32_t *insns = (uint32_t *)((uint8_t *)start + offset);
      uint8_t *old_pc = old_prologue_start + offset;

      if ((*insns & 0x1f000000) == 0x10000000)
      {
        // ADR or ADRP instruction
        int op = (*insns >> 31) & 0x1;
        // ADR (op == 0) calculate address in bytes
        // ADRP (op == 1) calculate address in 4096-byte pages
        // shift is either 0 or 12
        int shift = op * 12;
        int dest_reg = *insns & 0x0000001f;
        // the relative address is in bits 23..5 and 30..29
        int64_t rel_addr = SIGN_EXTEND(((*insns >> 3) & 0x001ffffc)
                                       | ((*insns >> 29) & 0x00000003), 21)
                           << shift;
        uint64_t base_addr = (uint64_t)old_pc & ~((1ull << shift) - 1);
        uint64_t abs_addr = base_addr + rel_addr;
        igprof_debug("patching adr%s r%d, .%+d at %p\n", op ? "p" : "",
                     dest_reg, rel_addr, insns);
        // replace the ADR(P) instruction with a LDR instruction
        *insns = ENCODE_LDR(dest_reg, (patch_insns - insns) * 4);
        *(uint64_t *)patch_insns = (uint64_t)abs_addr;
        patch_insns += 2;
      }
      else if ((*insns & 0x3b000000) == 0x18000000
               && (*insns & 0xc0000000) != 0xc0000000)
      {
        // LDR or LDRSW instruction
        // get the opc and V fields to determine the length of the literal
        int opc = (*insns >> 30) & 0x3;
        int v = (*insns >> 26) & 0x1;
        // length of literal in 32-bit words (1, 2 or 4)
        int literal_len = 1 << opc;
        if (opc == 2 && v == 0) //LDRSW
          literal_len = 1;
        // the relative address is in bits 23..5
        int64_t rel_addr = SIGN_EXTEND((*insns >> 3) & 0x001ffffc, 21);
        igprof_debug("patching ldr%s r%d, .%+d at %p\n",
                     opc == 2 && v == 0 ? "sw" : "", *insns & 0x1f, rel_addr,
                     insns);
        // patch the relative address to the patch area of the trampoline
        *insns &= 0xff80001f;
        *insns |= ((patch_insns - insns) << 5) & 0x00ffffe0;

        // copy the literal to the patch area
        memcpy((void *)patch_insns, (void *)(old_pc + rel_addr),
               literal_len * 4);
        patch_insns += literal_len;
      }
      else if ((*insns & 0x7c000000) == 0x14000000)
      {
        // B or BL instruction
        // the relative address is in bits 25..0
        int64_t rel_addr = SIGN_EXTEND((*insns << 2) & 0x0ffffffc, 28);
        igprof_debug("patching b%s, .%+d at %p\n",
                     *insns & 0x80000000 ? "l" : "", rel_addr, insns);
        // patch the relative address to the patch area of the trampoline
        *insns &= 0xfc000000;
        *insns |= (patch_insns - insns) & 0x03ffffff;
        redirect((void *&)patch_insns, (void *)(old_pc + rel_addr),
                 IgHook::JumpToTrampoline);
      }
      else if ((*insns & 0xff000010) == 0x54000000)
      {
        // B.cond instruction
        // the relative address is in bits 23..5
        int64_t rel_addr = SIGN_EXTEND((*insns >> 3) & 0x001ffffc, 21);
        const char *cond[] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
                              "hi", "ls", "ge", "lt", "gt", "le", "al", "nv"};
        igprof_debug("patching b.%s .%+d at %p\n", cond[*insns & 0xf],
                     rel_addr, insns);
        // patch the relative address to the patch area of the trampoline
        *insns &= 0xff80001f;
        *insns |= ((patch_insns - insns) << 5) & 0x00ffffe0;
        redirect((void *&)patch_insns, (void *)(old_pc + rel_addr),
                 IgHook::JumpToTrampoline);
      }
      else if ((*insns & 0x7e000000) == 0x34000000)
      {
        // CBZ or CBNZ instruction
        // the relative address is in bits 23..5
        int64_t rel_addr = SIGN_EXTEND((*insns >> 3) & 0x001ffffc, 21);
        igprof_debug("patching cb%sz r%d, .%+d at %p\n",
                     *insns & 0x01000000 ? "n" : "", *insns & 0x1f, rel_addr,
                     insns);
        // patch the relative address to the patch area of the trampoline
        *insns &= 0xff80001f;
        *insns |= ((patch_insns - insns) << 5) & 0x00ffffe0;
        redirect((void *&)patch_insns, (void *)(old_pc + rel_addr),
                 IgHook::JumpToTrampoline);
      }
      else if ((*insns & 0x7e000000) == 0x36000000)
      {
        // TBZ or TBNZ instruction
        // the relative address is in bits 18..5
        int64_t rel_addr = SIGN_EXTEND((*insns >> 3) & 0x0000fffc, 16);
        igprof_debug("patching tb%sz r%d, #%d, .%+d at %p\n",
                     *insns & 0x01000000 ? "n" : "", *insns & 0x1f,
                     ((*insns >> 25) & 0x20) | ((*insns >> 19) & 0x1f),
                     rel_addr, insns);
        // patch the relative address to the patch area of the trampoline
        *insns &= 0xfffc001f;
        *insns |= ((patch_insns - insns) << 5) & 0x0007ffe0;
        redirect((void *&)patch_insns, (void *)(old_pc + rel_addr),
                 IgHook::JumpToTrampoline);
      }
    }
    address = patch_insns;
  }
#endif
}

/** Link original function at @a address to @a trampoline.  The size
    of the previously parsed prologue is @a prologue bytes.  Replaces
    the initial sequence of function at @a address with a jump to @a
    trampoline.  If the jump instruction is less than @a prologue, the
    rest is filled up with "nop" instructions.  */
static void
patch(void *address, void *trampoline, int prologue)
{
  // FIXME: Not atomic, freeze all other threads!
  unsigned char *insns UNUSED = (unsigned char *) address;
  int i = redirect(address, trampoline, IgHook::JumpToTrampoline);
  for ( ; i < prologue; ++i)
  {
#if __i386__ || __x86_64__
    insns[i] = 0x90; // nop
#else
    // can't happen!
    igprof_abort();
#endif
  }
}

IgHook::Status
IgHook::hook(const char *function,
	     const char *version,
	     const char *library,
	     void *replacement,
	     int options /* = 0 */,
	     void **chain /* = 0 */,
	     void **original /* = 0 */,
	     void **trampoline)
{
  // For future compatibility -- call vs. jump, counting etc.
  if (options != 0)
    return ErrBadOptions;

  // Zero out variables
  if (chain) *chain = 0;
  if (original) *original = 0;
  if (trampoline) *trampoline = 0;

  // Lookup function
  Status s;
  void *sym = 0;
  if ((s = lookup(function, version, library, sym)) != Success)
    return s;

  if (original) *original = sym;

  // See if we understand it
  unsigned patches[TRAMPOLINE_SIZE];
  int prologue = parse(function, sym, patches);
  if (prologue < 0)
    return ErrPrologueNotRecognised;
  else if (prologue > TRAMPOLINE_SAVED)
    return ErrPrologueTooLarge;

  // Prepare trampoline
  void *tramp = 0;
  if ((s = allocate(tramp, sym)) != Success)
    return s;

  if (trampoline)
    *trampoline = tramp;

  if (version)
    igprof_debug("%s/%s (%p): instrumenting %d bytes into %p\n",
		 function, version, sym, prologue, tramp);
  else
    igprof_debug("%s (%p): instrumenting %d bytes into %p\n",
		 function, sym, prologue, tramp);

  prepare(tramp, replacement, chain, sym, prologue, patches);

  // Attach trampoline
  if ((s = protect(sym, true, prologue)) != Success)
  {
    release(tramp);
    return s;
  }

  patch(sym, tramp, prologue);

  // Restore privileges and flush caches
  // No: protect(tramp, false); -- segvs on linux, full page might not been allocated?
  protect(sym, false, prologue);
  flush(tramp);
  flush(sym);

  return Success;
}
