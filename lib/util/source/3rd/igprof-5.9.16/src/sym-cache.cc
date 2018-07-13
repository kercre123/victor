#include "sym-cache.h"
#include "walk-syms.h"
#include <memory.h>

/** Initialise a symbol translation buffer.  */
IgProfSymCache::IgProfSymCache(void)
{
  memset(bintable_, 0, sizeof(bintable_));
  memset(symtable_, 0, sizeof(symtable_));
  memset(symcache_, 0, sizeof(symcache_));
}

/** Destroy the symbol translation buffer.  */
IgProfSymCache::~IgProfSymCache(void)
{}

/** Get the symbol for an address if there is one.  */
IgProfSymCache::Symbol *
IgProfSymCache::symbolForAddress(void *address)
{
  Symbol *sym = symtable_[hash((uintptr_t) address, 32) & (SYMBOL_HASH-1)];
  while (sym)
  {
    if (sym->address == address)
      return sym;
    if ((char *) sym->address > (char *) address)
      return 0;
    sym = sym->next;
  }

  return 0;
}

void *
IgProfSymCache::roundAddressToSymbol(void *address)
{
  // Look up the address in call address to symbol address cache.
  void     *symaddr = address;
  SymCache **sclink = &symcache_[hash((uintptr_t) address, 32) & (SYMBOL_HASH-1)];
  SymCache *cached;
  while ((cached = *sclink))
  {
    // If we found it, return the saved address.
    if (cached->calladdr == address)
      return cached->symaddr;
    if ((char *) cached->calladdr > (char *) address)
      break;
    sclink = &cached->next;
  }

  // Look up the symbol for this call address.  If not present
  // add to the symbol cache, symbol table and library hashes.
  Symbol     *s;
  const char *binary;
  Symbol     sym = { 0, address, 0, 0, 0, 0, -1 };
  IgHookTrace::symbol(address, sym.name, binary, sym.symoffset, sym.binoffset);

  // Hook up the cache entry to sort order in the hash list.
  SymCache *next = *sclink;
  cached = *sclink = allocate<SymCache>();
  cached->next = next;
  cached->calladdr = address;
  cached->symaddr = symaddr;

  // Look up in the symbol table.
  bool found = false;
  Symbol **slink = &symtable_[hash((uintptr_t) symaddr, 32) & (SYMBOL_HASH-1)];
  while ((s = *slink))
  {
    if (s->address == sym.address)
    {
      found = true;
      break;
    }
    else if ((char *) s->address > (char *) sym.address)
      break;

    slink = &s->next;
  }

  // If not found, hook up
  if (! found)
  {
    // Hook up the symbol into sorted hash list order.
    sym.next = *slink;
    s = *slink = allocate<Symbol>();
    *s = sym;

    // Find and if necessary create the binary and hook into the symbol.
    Binary **blink = &bintable_[hash((uintptr_t) binary, 32) & (BINARY_HASH-1)];
    while (Binary *binobj = *blink)
    {
      if (binobj->name == binary)
      {
        s->binary = *blink;
        break;
      }
      blink = &binobj->next;
    }

    if (! s->binary)
    {
      Binary *binobj = *blink = s->binary = allocate<Binary>();
      binobj->name = binary;
      binobj->next = 0;
      binobj->id = -1;
    }
  }

  // Return the new symbol address.
  return cached->symaddr;
}

/** Return a symbol definition for an address. */
IgProfSymCache::Symbol *
IgProfSymCache::get(void *address)
{
  return symbolForAddress(roundAddressToSymbol(address));
}
